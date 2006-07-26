#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "lv2.h"

#ifdef WIN32
#define SYMBOL_EXPORT __declspec(dllexport)
#else
#define SYMBOL_EXPORT
#endif

#define AMP_URI       "http://codeson.net/plugins/amp"
#define AMP_GAIN      0
#define AMP_INPUT     1
#define AMP_OUTPUT    2

static LV2_Descriptor *ampDescriptor = NULL;

typedef struct {
	float *gain;
	float *input;
	float *output;
} Amp;


static void
cleanupAmp(LV2_Handle instance) {
	free(instance);
}


static void
connectPortAmp(LV2_Handle instance, unsigned long port,
			   void *data) {
	Amp *plugin = (Amp *)instance;

	switch (port) {
	case AMP_GAIN:
		plugin->gain = data;
		break;
	case AMP_INPUT:
		plugin->input = data;
		break;
	case AMP_OUTPUT:
		plugin->output = data;
		break;
	}
}


static LV2_Handle
instantiateAmp(const LV2_Descriptor *descriptor,
	    unsigned long s_rate, const char *path , const LV2_Host_Feature **features) {
	Amp *plugin_data = (Amp *)malloc(sizeof(Amp));

	return (LV2_Handle)plugin_data;
}


#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

static void
runAmp(LV2_Handle instance, unsigned long sample_count) {
	Amp *plugin_data = (Amp *)instance;

	const float gain = *(plugin_data->gain);
	const float * const input = plugin_data->input;
	float * const output = plugin_data->output;

	unsigned long pos;
	float coef = DB_CO(gain);

	for (pos = 0; pos < sample_count; pos++) {
		output[pos] = input[pos] * coef;
	}
}


static void
init() {
	ampDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	ampDescriptor->URI = AMP_URI;
	ampDescriptor->activate = NULL;
	ampDescriptor->cleanup = cleanupAmp;
	ampDescriptor->connect_port = connectPortAmp;
	ampDescriptor->deactivate = NULL;
	ampDescriptor->instantiate = instantiateAmp;
	ampDescriptor->run = runAmp;
}


SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(unsigned long index) {
	if (!ampDescriptor) init();

	switch (index) {
	case 0:
		return ampDescriptor;
	default:
		return NULL;
	}
}

