#include "modular80.hpp"

#include "dsp/digital.hpp"

struct Logistiker : Module {
	enum ParamIds {
		RATE_PARAM,
		R_PARAM,
		X_PARAM,
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLK_INPUT,
		RST_INPUT,
		R_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		X_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Logistiker() : x(0.0f),
		phase(0.0f)
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;
	void reset();
	void onReset() override;

private:
	float logistic(const float x, const float r);

	dsp::SchmittTrigger rstButtonTrigger;
	dsp::SchmittTrigger rstInputTrigger;
	dsp::SchmittTrigger clkTrigger;

	float x;
	float phase;
};

void Logistiker::reset() {
	onReset();
}

void Logistiker::onReset() {
	x = 0.0f;
	phase = 0.0f;
}

float Logistiker::logistic(const float x, const float r) {
	return(r * x * (1.0f - x));
}

void Logistiker::process(const ProcessArgs &args) {
	if (!outputs[X_OUTPUT].active) {
		return;
	}

	static bool doReset(false);

	if (rstButtonTrigger.process(params[RESET_PARAM].value) ||
	   (inputs[RST_INPUT].active && rstInputTrigger.process(inputs[RST_INPUT].value)))
	{
		doReset = true;
	}

	bool doStep(false);

	// External clock
	if (inputs[CLK_INPUT].active) {
		if (clkTrigger.process(inputs[CLK_INPUT].value)) {
			phase = 0.0f;
			doStep = true;
		}
	}
	else {
		// Internal clock
		phase += pow(2.0f, params[RATE_PARAM].value)/APP->engine->getSampleRate();
		if (phase >= 1.0f) {
			phase = 0.0f;
			doStep = true;
		}
	}

	if (doStep) {

		// Synchronize resetting x with steps.
		if (doReset) {
			x = params[X_PARAM].value;
			doReset = false;
		}

		const float r = clamp(params[R_PARAM].value + inputs[R_INPUT].value, 0.0f, 8.0f);

		// Don't let population die!
		x = clamp(logistic(x, r), 0.00001f, 1.0f);
	}

	outputs[X_OUTPUT].value = clamp(x * 10.0f, -10.0f, 10.0f);
}

struct LogistikerWidget : ModuleWidget {
	LogistikerWidget(Logistiker *module);
};

LogistikerWidget::LogistikerWidget(Logistiker *module) {
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Logistiker.svg")));

	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));

	//addParam(createParam<Davies1900hLargeBlackKnob>(Vec(18, 62), module, Logistiker::RATE_PARAM, -2.0f, 6.0f, 2.0f)); // 0.25..64
	//addParam(createParam<Davies1900hBlackKnob>(Vec(49, 140), module, Logistiker::R_PARAM, 0.0f, 8.0f, 3.56995f)); // default = onset of chaos
	//addParam(createParam<Davies1900hBlackKnob>(Vec(49, 206), module, Logistiker::X_PARAM, 0.0f, 0.5f, 0.0f));
	addParam(createParam<Davies1900hLargeBlackKnob>(Vec(18, 62), module, Logistiker::RATE_PARAM));
	addParam(createParam<Davies1900hBlackKnob>(Vec(49, 140), module, Logistiker::R_PARAM));
	addParam(createParam<Davies1900hBlackKnob>(Vec(49, 206), module, Logistiker::X_PARAM));

	addInput(createInput<PJ301MPort>(Vec(11, 146), module, Logistiker::R_INPUT));

	//addParam(createParam<TL1105>(Vec(15, 217), module, Logistiker::RESET_PARAM, 0, 1, 0));
	addParam(createParam<TL1105>(Vec(15, 217), module, Logistiker::RESET_PARAM));

	addInput(createInput<PJ301MPort>(Vec(54, 276), module, Logistiker::CLK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(11, 276), module, Logistiker::RST_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(33, 319), module, Logistiker::X_OUTPUT));

	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}

Model *modelLogistiker = createModel<Logistiker, LogistikerWidget>("Logistiker");
