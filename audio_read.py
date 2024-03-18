import librosa

read_data, sample_rate = librosa.load(
    'recycling.wav', sr=None, res_type='audioread',
    mono=True
)
print('SAMPLE_RATE', sample_rate)
print('DATA_SHAPE', read_data.shape)
