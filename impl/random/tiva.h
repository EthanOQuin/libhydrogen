#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"

static void
tiva_adc_init(void)
{
    // Enable the ADC0 module.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Wait for the ADC0 module to be ready.
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {
    }

    // Enable the first sample sequencer to capture the value of channel 0 when
    // the processor trigger occurs.
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_TS | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 0);
}

static uint32_t
tiva_adc_sample(void)
{
    uint32_t adc_sample;

    // Trigger the sample sequence.
    ADCProcessorTrigger(ADC0_BASE, 0);

    // Wait until the sample sequence has completed.
    while (!ADCIntStatus(ADC0_BASE, 0, false)) {
    }

    // Read the value from the ADC.
    ADCSequenceDataGet(ADC0_BASE, 0, &adc_sample);

    return adc_sample;
}

static int
hydro_random_init(void)
{
    const char       ctx[hydro_hash_CONTEXTBYTES] = { 'h', 'y', 'd', 'r', 'o', 'P', 'R', 'G' };
    hydro_hash_state st;
    uint16_t         ebits = 0;
    uint32_t         adc_sample;

    hydro_hash_init(&st, ctx, NULL);

    tiva_adc_init();

    while (ebits < 1024) {
        for (int i = 0; i < 10000; i++)
            ;
        adc_sample = tiva_adc_sample();

        hydro_hash_update(&st, &adc_sample, sizeof adc_sample);

        ebits++;
    }

    hydro_hash_final(&st, hydro_random_context.state, sizeof hydro_random_context.state);
    hydro_random_context.counter = ~LOAD64_LE(hydro_random_context.state);

    return 0;
}
