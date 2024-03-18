# PLC-group-project

ffmpeg used commands to generate uncompressed wav files  
1. stereo audio file  
`ffmpeg -i recycling.mp4 -acodec pcm_s16le recycling.wav` 
2. mono audio file  
`ffmpeg -i recycling.mp4 -acodec pcm_s16le -ac 1 recycling_mono.wav`

Command to build test the C code standalone:  
`gcc -ansi -pedantic -Wall -Werror -std=c11 main.c -o main && ./main recycling_mono.wav`

Command to run python testing code
1. Python environment setup  
`python3.10 -m venv venv`
2. Activate python env  
   `source venv/bin/activate`
3. Install dependencies   
   `python -m pip install -r requirements.txt`
4. Build shared object file for wav file parser to be used by python script    
`gcc -shared -o parse_wav.so -fPIC parse_wav.c`
5. Run testing script (it checks that wav_parse.c returns the same values as the librosa library)  
`python audio_test.py`