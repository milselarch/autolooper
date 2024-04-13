# Autolooper

A simple tool to help you extend music!

## Usage

Compile the code with  
`make ansi`

Run the program using  
`./main /path/to/input.wav /path/to/output.wav MIN_DURATION [START_TIME END_TIME]`

Notes:  
* Input file should be an uncompressed WAV file (see [Convert Audio to WAV](#convert-audio-to-wav) for more info).
* `MIN_DURATION`, `START_TIME` and `END_TIME` are in seconds and should be integers.
* If `START_TIME` and `END_TIME` are not provided, the program will attempt to find the music loop on its own. For best results, it is recommended for the input file to have at least 2 loops of the music.

Example:  
`./main input.wav output.wav 300 1 73`

### Convert Audio to WAV

Generating uncompressed wav files using ffmpeg:  
* stereo audio file  
`ffmpeg -i INPUT_FILE -acodec pcm_s16le OUTPUT_FILE`
* mono audio file  
`ffmpeg -i INPUT_FILE -acodec pcm_s16le -ac 1 OUTPUT_FILE`

### Validate Parser Results

Commands to run the Python testing code:  
1. Python environment setup  
`python3.10 -m venv venv`
2. Activate python env  
   `source venv/bin/activate`
3. Install dependencies   
   `python -m pip install -r requirements.txt`
4. Build shared object file for wav file parser to be used by python script    
`gcc -shared -o parse_wav.so -fPIC parse_wav.c`
5. Run testing script (it checks that parse_wav.c returns the same values as the librosa library)  
`python py_testing/audio_test.py`
