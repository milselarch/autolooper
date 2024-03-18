import ctypes
import numpy as np
import librosa

# import matplotlib.pyplot as plt
# Define the struct in Python


class WavParseResult(ctypes.Structure):
    _fields_ = [
        ("samples", ctypes.POINTER(ctypes.POINTER(ctypes.c_double))),
        ("num_frames", ctypes.c_ulong),
        ("sample_rate", ctypes.c_ulong),
        ("num_channels", ctypes.c_ulong),
        ("num_samples", ctypes.c_ulong)
    ]


# Assuming the compiled shared library is named example.so
lib = ctypes.CDLL('./parse_wav.so')
lib.read_wav_file.restype = WavParseResult
lib.read_wav_file.argtypes = [ctypes.c_char_p]
lib.free_wav_parse_result.argtypes = [WavParseResult]


def parse_wav(filepath: str) -> tuple[np.ndarray, int]:
    assert isinstance(filepath, str)
    c_str_filepath = ctypes.c_char_p(filepath.encode('utf-8'))
    result: WavParseResult = lib.read_wav_file(c_str_filepath)
    samples_arr = np.empty(
        (result.num_channels, result.num_samples), dtype=np.float16
    )

    for k in range(result.num_channels):
        for i in range(result.num_samples):
            samples_arr[k, i] = result.samples[k][i]

    lib.free_wav_parse_result(result)
    return samples_arr, result.sample_rate


if __name__ == '__main__':
    START = 15916
    OFFSET = 1000
    test_filepaths = ['./recycling_mono.wav', './recycling.wav']

    for filepath in test_filepaths:
        raw_data, sr = librosa.load(filepath, sr=None, mono=False)
        if len(raw_data.shape) == 1:
            raw_data = np.expand_dims(raw_data, axis=0)

        custom_raw_data, custom_sr = parse_wav(filepath)
        assert sr == custom_sr

        num_channels = custom_raw_data.shape[0]
        for channel_idx in range(num_channels):
            channel = raw_data[0]
            custom_channel = custom_raw_data[0]
            diffs = channel - custom_channel
            # make sure samples are only off by at most 0.001
            assert np.isclose(channel, custom_channel, atol=1e-3).all()
            print('MAX AMPLITUDE DIFF', max(abs(diffs)))

        """
        print('FLOAT_SAMPLE', raw_data[0][START], custom_raw_data[0][START])
        diffs = raw_data - custom_raw_data
        print(f"DIFFS MEAN={np.mean(diffs)} STD={np.std(diffs)}")
        print(filepath, raw_data.shape)

        plt.plot(raw_data[0][START:START+OFFSET], c='blue')
        plt.plot(custom_raw_data[0][START:START+OFFSET], c='red')
        plt.show()
        """