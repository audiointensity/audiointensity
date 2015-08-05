#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
	#include <io.h>
	#include <fcntl.h>
#endif

void read_wav_header(unsigned int *samp_rate, unsigned int *bits_per_samp,
					 unsigned int *num_samp);
int read_wav_data(int *data, unsigned int samp_rate,
				  unsigned int bits_per_samp, unsigned int num_samp);
int conv_bit_size(unsigned int in, int bps);

int findpeaks(signed long* v, int startind, int endind, int *peakx, int *peaky, int *peakprom);
void select_most_prom(int *peakprom, int *peakx, int *peaky, int n, int k);
void remove_close_peaks(int *peakx, int *peakprom, int npeaks, const int min_x_distance);

int max(int a, int b) {
	return a > b ? a : b;
}
int min(int a, int b) {
	return a < b ? a : b;
}
int filter(int *data, int i) {
	return abs(data[i] - data[i-4]);
}

int compare_ints(const void *a, const void *b) {
  return ( *(int*)a - *(int*)b );
}

const int DEBUG = 0, VERBOSE = 0;
int main(void)
{
	const int TARGET_FS = 11;
	const int MIN_PEAK_DISTANCE = 30*TARGET_FS; // Minimum distance (in samples, 1*TARGET_FS = 1 second) between detected points
	const int N_MOST_PROM = 5; // Number of points to detect
	const int FILTER_LEN1 = 2*TARGET_FS;
	const int FILTER_LEN2 = 10*TARGET_FS;
	const int MAX_LAG = 64;
	
	// Re-open stdin in binary mode
	#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	#else
	freopen(NULL, "rb", stdin);
	#endif
	unsigned int samp_rate, bits_per_samp, num_samp;
	read_wav_header(&samp_rate, &bits_per_samp, &num_samp);

	int *data = (int *) malloc(num_samp * sizeof(int));
	num_samp = read_wav_data(data, samp_rate, bits_per_samp, num_samp);
	if(DEBUG) 
		printf("samp_rate=[%d] bits_per_samp=[%d] num_samp=[%d]\n",
			samp_rate, bits_per_samp, num_samp);
	
	const int DOWN_SAMP_RATIO = samp_rate/TARGET_FS;
	const int NUM_DOWN_SAMP = num_samp/DOWN_SAMP_RATIO + 1;
	signed long bests[NUM_DOWN_SAMP];

	unsigned int i;	
	for (i = 4; i < num_samp; ++i) {
		if (i % DOWN_SAMP_RATIO <= 0) {
			bests[i/DOWN_SAMP_RATIO] = 0;
		}
		
		int p_hh = filter(data, i); // p_hh = filter([1 0 0 0 -1], 1, data);
		bests[i/DOWN_SAMP_RATIO] += p_hh;
		if (i % DOWN_SAMP_RATIO >= MAX_LAG+4) {
			 bests[i/DOWN_SAMP_RATIO] -= filter(data, i - MAX_LAG);
		}
	}
		
	signed long bestss[NUM_DOWN_SAMP];
	int t = 0, prev_t;
	for (i = 0; i < NUM_DOWN_SAMP+FILTER_LEN1/2; i++) {
		prev_t = t;
		t = min(max(0, i-FILTER_LEN1/2), NUM_DOWN_SAMP-1);
		bestss[t] = bestss[prev_t];
		if (i < NUM_DOWN_SAMP)
			bestss[t] += bests[i];
		if(i >= FILTER_LEN1)
			bestss[t] -= bests[i - FILTER_LEN1]; 
	}
	signed long bestsss[NUM_DOWN_SAMP];
	t = 0;
	for (i = 0; i < NUM_DOWN_SAMP+FILTER_LEN2/2; i++) {
		prev_t = t;
		t = min(max(0, i-FILTER_LEN2/2), NUM_DOWN_SAMP-1);
		bestsss[t] = bestsss[prev_t];
		if (i < NUM_DOWN_SAMP)
			bestsss[t] += bestss[i]/FILTER_LEN1;
		if(i >= FILTER_LEN2)
			bestsss[t] -= bestss[i - FILTER_LEN2]/FILTER_LEN1; 
	}
	
	int peakx[NUM_DOWN_SAMP], peaky[NUM_DOWN_SAMP], peakprom[NUM_DOWN_SAMP];
	int npeaks = findpeaks(bestsss, 0, NUM_DOWN_SAMP-1, peakx, peaky, peakprom);
	
	// Modify peak weights by the time at which they occur. Penalize early peaks.
	for (i = 0; i < npeaks; i++) {
		peakprom[i] *= 0.2 + (0.8*(float)peakx[i]/NUM_DOWN_SAMP);
	}
	// Eliminate small peaks that are closer than MIN_PEAK_DISTANCE samples to a bigger one.
	remove_close_peaks(peakx, peakprom, npeaks, MIN_PEAK_DISTANCE);
		
	// Place the N_MOST_PROM biggest peaks first in the arrays
	select_most_prom(peakprom, peakx, peaky, npeaks, N_MOST_PROM);
	
	int sortedpeaks[N_MOST_PROM];
	// Dump info about most prominent peaks
	for (i = 0; i < N_MOST_PROM; i++) {
		int seconds = (float)peakx[i]/ (num_samp/DOWN_SAMP_RATIO) * (float)num_samp/samp_rate;
		seconds = max(0, seconds -15); // Show timestamp starting a little before peak
		if(VERBOSE) printf("Peak detected at %5d s (%02d:%02d): %5d\n", seconds, seconds/60, seconds - 60*(seconds/60) , peakprom[i]/FILTER_LEN2/MAX_LAG);
		//else printf("%d\n", seconds);
		sortedpeaks[i] = seconds;
	}
	
	// Sort the most prominent peaks by time and print to stdout
	qsort (sortedpeaks, N_MOST_PROM, sizeof(int), compare_ints);
	for (i=0; i<N_MOST_PROM; i++) {
		printf("%d\n", sortedpeaks[i]);
    }
	
	return EXIT_SUCCESS;
}

// Place the k most prominent peaks first in the arrays (in-place)
void select_most_prom(int *peakprom, int *peakx, int *peaky, int n, int k) {
	int i, minIndex, minValue;
	for(i=0; i < k; i++) {
		minIndex = i;
		minValue = peakprom[i];
		int j;
		for(j = i+1; j < n; j++) {
			if (peakprom[j] > minValue) {
				minIndex = j;
				minValue = peakprom[j];
			}
		}
		//swap list[i] and list[minIndex]
		int temp = peakprom[i];
		peakprom[i] = peakprom[minIndex];
		peakprom[minIndex] = temp;
		temp = peakx[i];
		peakx[i] = peakx[minIndex];
		peakx[minIndex] = temp;
		temp = peaky[i];
		peaky[i] = peaky[minIndex];
		peaky[minIndex] = temp;
	}
}

// "Remove" peaks that are too close to another more prominent peak by setting their prominence to 0
void remove_close_peaks(int *peakx, int *peakprom, int npeaks, const int min_x_distance) {
	int i,j;
	for(i = 0; i < npeaks; i++) {
		for(j = 0; j < i; j++) {
			if (abs(peakx[i] - peakx[j]) < min_x_distance) {
				if(peakprom[i] < peakprom[j]) {
					peakprom[i] = 0;
					break;
				} else
					peakprom[j] = 0;
			}
		}		
	}
}

// Return the index of the largest element with index between startind and endind in v.
int indexOfMax(int *v, int startind, int endind) {
    int maxEl;
	int is_init = 0;
    int i, index;
    for(i = startind; i <= endind; i++) {
		if (!is_init || maxEl < v[i]) {
			maxEl = v[i];
			index = i;
			is_init = 1;
		}
    }
    return index;
}
// Write x, y (height), and prominence of peaks found in the values of v.
int findpeaks(signed long* v, int startind, int endind, int *peakx, int *peaky, int *peakprom) {
    if (startind > endind) {
		if(DEBUG)printf("(in findpeaks()) Warning: startindex > endindex");
		return 0;
    }
	const int MAX_PEAKS = endind - startind + 1;
	if(DEBUG)printf("len:%d, ", MAX_PEAKS);
	int peaks_size = 0;
	int minlocs[MAX_PEAKS];
	int minlocs_size = 0;
	int i;
    for (i = startind+1; i < endind-1; i++) {
		if (v[i-1] < v[i] && v[i] >= v[i+1]) {
			if (minlocs_size == 0 && peaks_size == 0) {
				// Ensure that there's a minimum before all peaks
				minlocs[minlocs_size++] = v[startind]; 
			}
			peaky[peaks_size] = v[i];
			peakx[peaks_size] = i;
			peaks_size++;
		}
		if (v[i-1] > v[i] && v[i] <= v[i+1])
			minlocs[minlocs_size++] = v[i]; 
    }
    if (minlocs_size == peaks_size) {
		// Ensure that there's a minimum after all peaks
		minlocs[minlocs_size++] = v[endind];// Add the last point as minimum
	}
	
    int N = peaks_size;
	if(DEBUG) printf("peaks:%d\n", N);
	for(i = 0; i < N; i++)
		peakprom[i] = 0;
	int s;
    for(s = 0; s < N; s++) { 
		if(DEBUG)printf("\r%d", s);
		int e;
		for(e = s+1; e < N+1; e++) {
			// Current interval: [s, e]
			// Get the highest peak of current interval
			int h = indexOfMax(peaky, s, e);
			// Update prominence of the peak
			peakprom[h] = max(peakprom[h], peaky[h] - max(minlocs[s], minlocs[e]));
		}
    }
	if(DEBUG)printf("\n");
	return N;
}

// Read WAV header information (like in a .wav file) from stdin
void read_wav_header(unsigned int *samp_rate, unsigned int *bits_per_samp,
unsigned int *num_samp)
{
	unsigned char buf[256];

	/* ChunkID (RIFF for little-endian, RIFX for big-endian) */
	fread(buf, 1, 4, stdin);
	buf[4] = '\0';
	if (strcmp((char*)buf, "RIFF")) exit(EXIT_FAILURE);
	/* ChunkSize */
	fread(buf, 1, 4, stdin);
	/* Format */
	fread(buf, 1, 4, stdin);
	buf[4] = '\0';
	if (strcmp((char*)buf, "WAVE")) exit(EXIT_FAILURE);

	/* Subchunk1ID */
	fread(buf, 1, 4, stdin);
	buf[4] = '\0';
	if (strcmp((char*)buf, "fmt ")) exit(EXIT_FAILURE);
	/* Subchunk1Size (16 for PCM) */
	fread(buf, 1, 4, stdin);
	if (buf[0] != 16 || buf[1] || buf[2] || buf[3]) exit(EXIT_FAILURE);
	/* AudioFormat (PCM = 1, other values indicate compression) */
	fread(buf, 1, 2, stdin);
	if (buf[0] != 1 || buf[1]) exit(EXIT_FAILURE);
	/* NumChannels (Mono = 1, Stereo = 2, etc) */
	fread(buf, 1, 2, stdin);
	unsigned int num_ch = buf[0] + (buf[1] << 8);
	if (num_ch != 1) exit(EXIT_FAILURE);
	/* SampleRate (8000, 44100, etc) */
	fread(buf, 1, 4, stdin);
	*samp_rate = buf[0] + (buf[1] << 8) +
	(buf[2] << 16) + (buf[3] << 24);
	/* ByteRate (SampleRate * NumChannels * BitsPerSample / 8) */
	fread(buf, 1, 4, stdin);
	const unsigned int byte_rate = buf[0] + (buf[1] << 8) +
	(buf[2] << 16) + (buf[3] << 24);
	/* BlockAlign (NumChannels * BitsPerSample / 8) */
	fread(buf, 1, 2, stdin);
	const unsigned int block_align = buf[0] + (buf[1] << 8);
	/* BitsPerSample */
	fread(buf, 1, 2, stdin);
	*bits_per_samp = buf[0] + (buf[1] << 8);
	if (byte_rate != ((*samp_rate * num_ch * *bits_per_samp) >> 3))
	exit(EXIT_FAILURE);
	if (block_align != ((num_ch * *bits_per_samp) >> 3))
	exit(EXIT_FAILURE);

	/* Subchunk2ID */
	fread(buf, 1, 4, stdin);
	buf[4] = '\0';
	//printf("s%ss", buf);
	if (!strcmp((char*)buf, "LIST")) {
		fread(buf, 1, 4, stdin);
		const unsigned int list_chunk_length = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);

		/* Skip LIST chunk */
		fread(buf, 1, list_chunk_length, stdin);
		
		/* Subchunk2ID */
		fread(buf, 1, 4, stdin);
		buf[4] = '\0';
		//printf("%d", buf);
	}
		
	if (strcmp((char*)buf, "data"))	exit(EXIT_FAILURE);
	/* Subchunk2Size (NumSamples * NumChannels * BitsPerSample / 8) */
	fread(buf, 1, 4, stdin);
	const unsigned int subchunk2_size = buf[0] + (buf[1] << 8) +
	(buf[2] << 16) + (buf[3] << 24);
	*num_samp = (subchunk2_size << 3) / (
	num_ch * *bits_per_samp);
}

// Read WAV sound datan (like in a .wav file) from stdin and place it in data[]
int read_wav_data(int *data, unsigned int samp_rate,
unsigned int bits_per_samp, unsigned int num_samp)
{
	unsigned char buf;
	unsigned int i, j;
	for (i=0; i < num_samp; ++i) {
		if ((i % 1000) == 0) {
			fprintf(stderr, "\rMinutes decoded: %4.2f\r", i/samp_rate/60.0);
		}
		unsigned int tmp = 0;
		for (j=0; j != bits_per_samp; j+=8) {
			if(!fread(&buf, 1, 1, stdin)) { // stdin ended short of the given file size. Must be coming streaming from FFmpeg etc.
				num_samp = i; // Will also end the outer loop
				break;
			}
			tmp += buf << j;
		}
		
		data[i] = conv_bit_size(tmp, bits_per_samp);
	}
	return num_samp;
}
// Convert unsigned to signed integers
int conv_bit_size(unsigned int in, int bps)
{
	const unsigned int max = (1 << (bps-1)) - 1;
	return in > max ? in - (max<<1) : in;
}