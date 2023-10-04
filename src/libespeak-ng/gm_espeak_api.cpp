#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <espeak-ng/speak_lib.h>

#define GM_func extern "C" __declspec(dllexport)

int textSize = 500;
bool isPlaying = false, isLoaded = false;
char* avaliableVoices = NULL;
std::mutex mutexSynth;

char* listVoices() {
	const espeak_VOICE * const *voices;
	const espeak_VOICE* voiceCurrent;
	int size, i = 0;
	const int bufferSize = 2048;
	char buffer[bufferSize];
	size_t pos = 0;

	voices = espeak_ListVoices(NULL);
	size = sizeof(espeak_VOICE);

	do {
		std::string id, name, language, gender, age;

		voiceCurrent = voices[i++];
		if (voiceCurrent == NULL) break;

		id = voiceCurrent->identifier;
		name = voiceCurrent->name;
		language = voiceCurrent->languages;
		age = std::to_string(voiceCurrent->age);

		switch (voiceCurrent->gender) {
		case 1:
			gender = "male";
			break;
		case 2:
			gender = "female";
			break;
		default:
			gender = "none";
			break;
		}

		pos += snprintf(buffer + pos, bufferSize - pos, "%s %s %s %s %s, ", id.c_str(), name.c_str(), language.c_str(), gender.c_str(), age.c_str());
	} while (true);

	char* result = new char[pos + 1];
	snprintf(result, pos + 1, "%s", buffer);
	return result;
}

GM_func double espeakGM_Synth(char *text, double positionType, double position, double endPosition) {
	std::thread synth_thread([&]() {
		std::string arg0(text);
		espeak_ERROR r;
		espeak_POSITION_TYPE _positionType;

		while (!isLoaded) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		_positionType = static_cast<espeak_POSITION_TYPE>(static_cast<int>(positionType));
		mutexSynth.lock();
		isPlaying = true;
		r = espeak_Synth(&arg0[0], textSize, position, _positionType, endPosition, espeakCHARS_AUTO, NULL, NULL);
		isPlaying = false;
		mutexSynth.unlock();

		return r;
	});

	synth_thread.detach();
	
	return 0;
}

GM_func double espeakGM_Initialize(double output, double buflength, double options) {
	espeak_AUDIO_OUTPUT _output;
	int r;

	_output = static_cast<espeak_AUDIO_OUTPUT>(static_cast<int>(output));
	r = espeak_Initialize(_output, buflength, NULL, options);

	std::thread thread_loaded([&]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		delete[] avaliableVoices;
		avaliableVoices = listVoices();
		isLoaded = true;
	});

	thread_loaded.detach();
	
	return r;
}

GM_func double espeakGM_SetVoice(char *voice) {
	std::thread thread_set_voice([&]() {
		 mutexSynth.lock();
		 std::string arg0(voice);

		 espeak_SetVoiceByName(&arg0[0]);
		 mutexSynth.unlock();
		 return 0;
     });

	 thread_set_voice.detach();
	 return 0;
}

GM_func double espeakGM_IsPlaying() {
	return isPlaying ? 1 : 0;
}

GM_func double espeakGM_SetRate(double value, double relative) {
	return espeak_SetParameter(espeakRATE, value, relative);
}

GM_func double espeakGM_SetVolume(double value, double relative) {
	return espeak_SetParameter(espeakVOLUME, value, relative);
}

GM_func double espeakGM_SetPitch(double value, double relative) {
	return espeak_SetParameter(espeakPITCH, value, relative);
}

GM_func double espeakGM_SetRange(double value, double relative) {
	return espeak_SetParameter(espeakRANGE, value, relative);
}

GM_func double espeakGM_SetCapitals(double value, double relative) {
	return espeak_SetParameter(espeakCAPITALS, value, relative);
}

GM_func double espeakGM_SetWordGap(double value, double relative) {
	return espeak_SetParameter(espeakWORDGAP, value, relative);
}

GM_func double espeakGM_SetPonctuation(double value, double relative) {
	return espeak_SetParameter(espeakPUNCTUATION, value, relative);
}

GM_func char* espeakGM_GetCurrentVoice() {
	static char voiceInfo[256];
	if (voiceInfo == NULL) {
		return NULL;
	}
	espeak_VOICE* voice = espeak_GetCurrentVoice();
	if (voice == NULL) {
		return NULL;
	}
	char gender[10];
	switch (voice->gender) {
	case 1:
		strcpy(gender, "male");
		break;
	case 2:
		strcpy(gender, "female");
		break;
	default:
		strcpy(gender, "none");
		break;
	}
	snprintf(voiceInfo, 256, "%s %s %s %s %d",
		voice->identifier, voice->name, voice->languages, gender, voice->age);
	return voiceInfo;
}

GM_func char* espeakGM_ListVoices() {
	return avaliableVoices;
}

GM_func double espeakGM_Terminate() {
	isLoaded = false;
	return espeak_Terminate();
}

GM_func double espeakGM_SetTextSize(double size) {
	textSize = size;

	return 0;
}

GM_func double espeakGM_GetTextSize() {
	return textSize;
}