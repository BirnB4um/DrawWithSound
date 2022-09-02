#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <math.h>
#include <thread>
using namespace std;

//NOTE: only works for linear waterfall (not logarithmic)

// ===== CONFIGURABLE VARIABLES =====
uint64_t additional_time = 10000;  //additional empty space at beginning and at end (44100 = 1 second)
int nr_threads = 16;               //number of threads used to compute data
float screen_w = 965;              //width of waterfall
float screen_h = 1075;             //height of waterfall
float max_time = 22;               //seconds of waterfall-length
float min_freq = 500;              //start frequency
int max_freq = 7500;               //end frequency
// ==================================

short* sample;
float* amp_list;
float screen_ratio = screen_w / screen_h;
int d_freq = max_freq - min_freq;
int img_w;
int img_h;
long long dt_line;
long process = 0;

short sinwave(double time, double freq, double amp) {
	return short(amp * 32767 * sin(3.141592653 * 2 * time * freq / 44100.0f));
}

void th_run(int ind, int y, int h) {
	long long t;
	long s;
	double all_amp;
	for (long long i = y; i < y + h; i++) {
		all_amp = 0;
		for (unsigned int j = 0; j < d_freq; j++) {
			all_amp += amp_list[i * d_freq + j];
		}
		for (long long n = 0; n < dt_line; n++) {
			s = 0;
			t = i * dt_line + n;
			for (unsigned int j = 0; j < d_freq; j++) {
				s += sinwave(t + j * 999999, min_freq + j, amp_list[i * d_freq + j]);
			}
			s = s / all_amp;
			sample[additional_time + t] = s;
		}
		process++;
		if (ind == 0)
			cout << process << " / " << img_h << "\n";
	}
}

void normalize(short* samples, size_t size) {
	short max_amp = 0;
	for (size_t i = 0; i < size; i++) {
		if (std::abs(samples[i]) > max_amp) {
			max_amp = std::abs(samples[i]);
		}
	}
	float factor = 32767.0f / max_amp;
	for (size_t i = 0; i < size; i++) {
		samples[i] *= factor;
	}
}

int main() {
	cout << "NOTE: only works for linear waterfall (not logarithmic)" << endl;
	cout << "image path: ";

	std::string image_path;
	cin >> image_path;

	sf::Image image_to_draw;
	if (!image_to_draw.loadFromFile(image_path)) {
		cout << "ERROR: couldnt open image" << endl;
	}
	img_w = image_to_draw.getSize().x;
	img_h = image_to_draw.getSize().y;

	nr_threads = nr_threads > img_h ? img_h : nr_threads;

	amp_list = new float[d_freq * img_h];

	cout << "calculating frequencies..." << endl;

	//set amplitudes for each frequency
	for (int y = img_h - 1; y >= 0; y--) {
		for (unsigned int x = 0; x < d_freq; x++) {
			amp_list[(img_h - 1 - y) * d_freq + x] = float(image_to_draw.getPixel(int(float(img_w) * float(x) / d_freq), y).r) / 255;
		}
	}

	float secs = max_time * ((float(d_freq) / 8000) * screen_w / screen_h) / (float(img_w) / img_h);
	float sec_pro_line = secs / img_h;
	dt_line = sec_pro_line * 44100;

	uint64_t sample_count = dt_line * img_h;
	sample = new short[sample_count + 2 * additional_time];

	cout << "writing to buffer..." << endl;

	//add empty space to audio
	for (unsigned int i = 0; i < additional_time; i++) {
		sample[i] = 0;
	}

	//write data to audio in threads
	std::vector<thread> th_vec;
	for (int i = 0; i < nr_threads; i++) {
		int start = i * int(img_h / nr_threads);
		int length = int(img_h / nr_threads) + 1;
		th_vec.push_back(thread(th_run, i, start, length));
	}
	for (int i = 0; i < nr_threads; i++) {
		th_vec[i].join();
	}

	//add empty space to audio end
	for (unsigned int i = additional_time + sample_count; i < 2 * additional_time + sample_count; i++) {
		sample[i] = 0;
	}

	cout << "normalize samples..." << endl;
	normalize(sample, 2 * additional_time + sample_count);

	//========================

	sf::Sound sound;
	sf::SoundBuffer buffer;
	buffer.loadFromSamples(sample, sample_count + 2 * additional_time, 1, 44100);
	sound.setBuffer(buffer);
	buffer.saveToFile("out_sound.wav");

	delete[] sample;
	delete[] amp_list;

	cout << "finished!" << endl;
	return 0;
}