/*
*********************************************************************
*
*   IIR Oscillator and filter example
*
*   This example program generates a sinusoidal signal, disturbed by 
*   another sine signal of higher frequency.
*   An IIR low pass filter is then used to filter the disturbed signal
*   to reconstruct the original sine wave.
*
*   2011 - Tecnologix srl
*
*********************************************************************
*/

#include <stdlib.h>
#include <rtl.h>
#include <stdio.h>

#include "arm_math.h"
#include "sine_generator.h"
#include "rtxtime.h"
#include "low_pass_filter.h"

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
// <e>Oscillator Configuration
// <i>This is a demonstration on how to use the Configuration Wizard to create an easy configuration menu.

#define ENABLE_CONFIG 1
// =============================
//   <o>Oscillator Sampling Frequency [Hz] <1000-10000>
//   <i> Set the oscillator sampling frequency.
//   <i> Default: 5000  (5 KHz)
#define SAMPLING_FREQ 1000  // generating task (5 KHz)

//   <o>Noise Frequency [Hz] <50-10000>
//   <i> Set the noise signal frequency.
//   <i> Default: 1500 Hz
#define NOISE_FREQ    50  // noise (2 KHz)

//   <o>Signal Frequency [Hz] <10-1000>
//   <i> Set the signal frequency.
//   <i> Default: 330 Hz
#define SIGNAL_FREQ    10  // disturbed signal (250 Hz)

// </e>
//------------- <<< end of configuration section >>> -----------------------


sine_generator_q15_t Signal_set;
sine_generator_q15_t Noise_set;

q15_t sine;
q15_t noise;
q15_t disturbed;
q15_t filtered;

OS_TID sine_gen_tid;
OS_TID noise_gen_tid;
OS_TID disturb_gen_tid;
OS_TID filter_tsk_tid;
OS_TID sync_tsk_tid;

/*
*********************************************************************
*
* Sine Generator task
*
*********************************************************************
*/

__task void sine_gen(void)
{
  while(1)
  {
    os_evt_wait_and(0x0001, 0xFFFF);
    sine = sine_calc_sample_q15(&Signal_set) / 2;
    os_evt_set(0x0001, noise_gen_tid);
  }
}

/*
*********************************************************************
*
* Noise Sine Generator task
*
*********************************************************************
*/

__task void noise_gen(void)
{
  while(1)
  {
    os_evt_wait_and(0x0001, 0xFFFF);
    noise = sine_calc_sample_q15(&Noise_set) / 6;
    os_evt_set(0x0001, disturb_gen_tid);
  }
}

/*
*********************************************************************
*
* 1st Disturb Generator task
*
* Adds Noise to Signal
*
*********************************************************************
*/

__task void disturb_gen(void)
{
  while(1)
  {
    os_evt_wait_and(0x0001, 0xFFFF);
    disturbed = sine + noise;
    os_evt_set(0x0001, filter_tsk_tid);
  }
}

/*
*********************************************************************
*
* Filter task
*
* Extract Signal from disturbed signal by filtering out Noise
*
*********************************************************************
*/

__task void filter_tsk(void)
{
  while(1)
  {
    os_evt_wait_and(0x0001, 0xFFFF);
    filtered = low_pass_filter(&disturbed); 
  }
}

/*
*********************************************************************
*
* Synchronization task
*
* Periodically starts the signal generation process
*
*********************************************************************
*/

__task void sync_tsk(void)
{
  os_itv_set (1);

  while(1)
  {
    os_evt_set(0x0001, sine_gen_tid);
    os_itv_wait ();
  }
}

/*
*********************************************************************
*
* Initialization task
*
*********************************************************************
*/

__task void main_tsk(void)
{
  // compute coefficients for IIR sine generators
  sine_generator_init_q15(&Signal_set, SIGNAL_FREQ, SAMPLING_FREQ);
  sine_generator_init_q15(&Noise_set, NOISE_FREQ, SAMPLING_FREQ);
  printf ("Sine Generator Initialised\n\r");

  // initialize low pass filter
  low_pass_filter_init();
  printf ("Low Pass Filter Initialised\n\r");

  // initialize the timing system to activate the four tasks 
  // of the application program
  filter_tsk_tid = os_tsk_create(filter_tsk, 1);
  printf ("filter_tsk Task Initialised\n\r");
  disturb_gen_tid = os_tsk_create(disturb_gen, 1);
  printf ("disturb_gen Task Initialised\n\r");
  noise_gen_tid = os_tsk_create(noise_gen, 1);
  printf ("noise_gen Task Initialised\n\r");
  sine_gen_tid = os_tsk_create(sine_gen, 1);
  printf ("sine_gen Task Initialised\n\r");
  sync_tsk_tid = os_tsk_create(sync_tsk, 1); 
  printf ("sync_tsk Task Initialised\n\r");
  printf ("Application Running\n\r");

  while(1)
    os_tsk_delete_self();
}

/*
*********************************************************************
*
* Initialization function
*
*********************************************************************
*/

int main(void)
{
  os_sys_init(main_tsk);
}
