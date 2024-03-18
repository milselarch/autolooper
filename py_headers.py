import audioread

import wave
import struct

reader = audioread.audio_open('recycling_mono.wav')
with reader as input_file:
    print(dir(input_file))
    print(input_file.duration * input_file.samplerate)

file = open('recycling_mono.wav', 'rb')
data = file.read()
print('DATA', list([chr(x) for x in data[:100]]))

file = open('recycling_mono.wav', 'rb')

file.seek(0)
file_obj = wave.open(file)
print('FRAMES', file_obj.getnframes())

file = open('recycling_mono.wav', 'rb')
chunk_name = file.read(4)
print(chunk_name)
raw_chunk_size = file.read(4)

chunksize = struct.unpack_from('<L', raw_chunk_size)[0]
print('CHUNK_SIZE', chunksize)

