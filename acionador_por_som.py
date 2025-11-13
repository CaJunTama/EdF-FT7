import sounddevice as sd
import numpy as np
import pyautogui
from scipy.fft import rfft, rfftfreq
import threading
import time

# ==== PARÂMETROS AJUSTÁVEIS ====
AMPLITUDE_THRESHOLD = 0.0003         # Volume mínimo
FREQ_RANGE = (50, 10000)            # Frequência alvo (em Hz)
CLIQUE_COOLDOWN = 1.5                # Tempo mínimo entre cliques
DURACAO_ANALISE = 0.3                # Janela de tempo para escuta
# ==============================

sample_rate = 44100
ultimo_clique = 0

def detectar_som(indata, frames, time_info, status):
    global ultimo_clique

    audio = indata[:, 0]
    volume_norm = np.linalg.norm(audio) / len(audio)
    print(f"[DEBUG] Volume: {volume_norm:.6f}")

    if volume_norm < AMPLITUDE_THRESHOLD:
        return  # Som muito fraco

    # FFT para analisar frequências
    fft_result = np.abs(rfft(audio))
    freqs = rfftfreq(len(audio), 1 / sample_rate)
    freq_pico = freqs[np.argmax(fft_result)]

    print(f"[DEBUG] Pico de frequência: {int(freq_pico)} Hz")

    if FREQ_RANGE[0] <= freq_pico <= FREQ_RANGE[1]:
        agora = time.time()
        if agora - ultimo_clique > CLIQUE_COOLDOWN:
            print(f"[INFO] Som detectado! Volume: {volume_norm:.6f} | Freq: {int(freq_pico)}Hz")
            pyautogui.click()
            ultimo_clique = agora
    else:
        print(f"[DEBUG] Frequência fora da faixa ({int(freq_pico)}Hz)")

INPUT_DEVICE = 1  # Microfone funcional identificado por você

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
    t = threading.Thread(target=iniciar_escuta)
    t.daemon = True
    t.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[INFO] Encerrado pelo usuário.")
