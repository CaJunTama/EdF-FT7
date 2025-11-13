import sounddevice as sd
import numpy as np
import pyautogui
from scipy.fft import rfft, rfftfreq
import multiprocessing
import time

# ==== PARÂMETROS AJUSTÁVEIS ====
AMPLITUDE_THRESHOLD = 0.0002         # Volume mínimo
FREQ_RANGE = (50, 10000)             # Frequência alvo (em Hz)
CLIQUE_COOLDOWN = 1.5                # Tempo mínimo entre cliques
DURACAO_ANALISE = 0.3                # Janela de tempo para escuta
# ==============================

sample_rate = 44100
ultimo_clique = multiprocessing.Value('d', 0.0)

def realizar_clique(shared_time):
    pyautogui.click()
    shared_time.value = time.time()

def detectar_som(indata, frames, time_info, status):
    audio = indata[:, 0]
    volume_norm = np.linalg.norm(audio) / len(audio)
    print(f"[DEBUG] Volume: {volume_norm:.6f}")

    if volume_norm < AMPLITUDE_THRESHOLD:
        return

    fft_result = np.abs(rfft(audio))
    freqs = rfftfreq(len(audio), 1 / sample_rate)
    freq_pico = freqs[np.argmax(fft_result)]

    print(f"[DEBUG] Pico de frequência: {int(freq_pico)} Hz")

    if FREQ_RANGE[0] <= freq_pico <= FREQ_RANGE[1]:
        agora = time.time()
        if agora - ultimo_clique.value > CLIQUE_COOLDOWN:
            print(f"[INFO] Som detectado! Volume: {volume_norm:.6f} | Freq: {int(freq_pico)}Hz")
            p = multiprocessing.Process(target=realizar_clique, args=(ultimo_clique,))
            p.start()
    else:
        print(f"[DEBUG] Frequência fora da faixa ({int(freq_pico)}Hz)")

INPUT_DEVICE = 1  # Índice do microfone funcional

def iniciar_escuta():
    with sd.InputStream(callback=detectar_som,
                        channels=1,
                        samplerate=sample_rate,
                        device=INPUT_DEVICE,
                        blocksize=int(sample_rate * DURACAO_ANALISE)):
        print("[INFO] Escutando o microfone... Pressione Ctrl+C para sair.")
        while True:
            time.sleep(0.1)

if __name__ == "__main__":
    multiprocessing.set_start_method('spawn')  # Compatível com Windows
    try:
        iniciar_escuta()
    except KeyboardInterrupt:
        print("\n[INFO] Encerrado pelo usuário.")
