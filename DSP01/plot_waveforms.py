from pathlib import Path
import wave

import matplotlib.pyplot as plt
import numpy as np


def read_wav(file_name):
    with wave.open(str(file_name), "rb") as wav_file:
        if wav_file.getnchannels() != 2 or wav_file.getsampwidth() != 2:
            raise ValueError(f"Expected stereo 16-bit WAV: {file_name}")
        sample_rate = wav_file.getframerate()
        frame_count = wav_file.getnframes()
        samples = np.frombuffer(wav_file.readframes(frame_count), dtype="<i2")
    return sample_rate, samples.reshape(-1, 2)


def main():
    script_folder = Path(__file__).resolve().parent
    audio_folder = script_folder / "audio"
    output_folder = script_folder / "figure"
    output_folder.mkdir(exist_ok=True)

    for sample_rate in (4000, 8000, 16000):
        for frequency in (100, 400, 3000):
            input_file = audio_folder / (
                f"sincos_fs{sample_rate}_f{frequency}_L1.0.wav"
            )
            filtered_file = audio_folder / (
                f"filtered_sincos_fs{sample_rate}_f{frequency}_L1.0.wav"
            )
            input_rate, input_samples = read_wav(input_file)
            output_rate, output_samples = read_wav(filtered_file)

            if input_rate != output_rate or input_rate != sample_rate:
                raise ValueError(f"Sample-rate mismatch for {input_file}")

            alias_frequency = abs(
                (frequency + sample_rate / 2) % sample_rate - sample_rate / 2
            )
            available_count = min(len(input_samples), len(output_samples))
            if alias_frequency == 0:
                count = available_count
            else:
                count = min(
                    available_count,
                    round(sample_rate * 5 / alias_frequency),
                )
            time = np.arange(count) / sample_rate
            figure, axes = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

            for channel, title in enumerate(("Left channel: sine", "Right channel: cosine")):
                axes[channel].plot(
                    time, input_samples[:count, channel],
                    label="Before filtering", color="#2878B5", linewidth=1,
                )
                axes[channel].plot(
                    time, output_samples[:count, channel],
                    label="After filtering", color="#D95F02", linewidth=1,
                )
                axes[channel].set_title(title)
                axes[channel].set_ylabel("Amplitude")
                axes[channel].grid(True, alpha=0.3)
                axes[channel].legend()

            axes[1].set_xlabel("Time (seconds)")
            title = f"RC Low-Pass Filter: {frequency} Hz, fs = {sample_rate} Hz"
            if alias_frequency != frequency:
                title += f" (sampled as {alias_frequency:g} Hz)"
            figure.suptitle(title)
            figure.tight_layout()
            output_file = output_folder / f"waveform_fs{sample_rate}_f{frequency}.png"
            figure.savefig(output_file, dpi=150)
            plt.close(figure)
            print(f"Saved {output_file}")


if __name__ == "__main__":
    main()