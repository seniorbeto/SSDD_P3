from concurrent.futures import ThreadPoolExecutor, as_completed
import subprocess
import time
import random
import re

num_clientes = 50
num_comandos_por_cliente = 50

SERVER_ADDR = "localhost"
SERVER_PORT = 4444
def __run(commands):
    """
    Ejecuta la aplicación del cliente con una secuencia de comandos.
    :param commands: Lista de comandos (strings) que se enviarán a la shell.
    :return: Tupla (salida, error) de la ejecución.
    """
    input_data = "\n".join(commands) + "\n"
    process = subprocess.Popen(["env",f"IP_TUPLAS={SERVER_ADDR}",f"PORT_TUPLAS={SERVER_PORT}", "./cmake-build-release/app-cliente"],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               text=True)
    out, err = process.communicate(input_data)
    return out, err

def get_random_command():
    num = random.randint(0, 100000000)
    comandos = [
        f'set {num} valor_{num} 3 1.0 2.0 3.0 10 5',
        f'get {num}',
        f'modify {num} mod_{num} 3 4.0 5.0 6.0 20 10',
        f'get {num}',
        f'delete {num}',
        f'exist {num}',
    ]
    return random.choice(comandos)

def operaciones_cliente(clave):
    comandos = []
    comandos.append("destroy")
    for i in range(num_comandos_por_cliente):
        comandos.append(get_random_command())
    comandos.append("exit")

    start = time.time()
    salida, error = __run(comandos)
    end = time.time()
    return clave, salida, error, end - start

medias = []
throughputs = []
errors = []

for i in range(1, num_clientes):
    resultados = []
    tiempos = []
    with ThreadPoolExecutor(max_workers=i) as executor:
        futuros = [executor.submit(operaciones_cliente, clave) for clave in range(i)]
        for futuro in as_completed(futuros):
            num, salida, error, tiempo = futuro.result()
            resultados.append((num, salida, error, tiempo))
            tiempos.append(tiempo)

    # MEDIA DEL TIEMPO
    media = sum(tiempos) / len(tiempos)
    media_por_peticion = media / num_comandos_por_cliente

    # THROUGHPUT
    throughput = len(tiempos) / sum(tiempos)
    throughput_por_peticion = throughput * num_comandos_por_cliente

    # Calculamos el número de errores que se han producido
    errores = len([r for r in resultados if r[2]]) / len(resultados) * 100
    for r in resultados:
        if r[2]:
            print(f"Error en la petición {r[0]}: {r[2]}")
    errors.append(errores)

    medias.append(media_por_peticion)
    throughputs.append(throughput_por_peticion)

# esto nos dará la variación en la media de ejcución de comandos
# y el throughput en función del número de clientes operando
# concurrentemente en el servidor.

print(errors)

import matplotlib.pyplot as plt

plt.figure(figsize=(12, 6))

ax1 = plt.subplot(1, 2, 1)
ax1.set_title(f"Throughput y Errores para {num_clientes} clientes concurrentes ({num_comandos_por_cliente} comandos/cliente)")
ax1.plot(range(1, num_clientes), throughputs, 'o-', label="Throughput", color="blue")
ax1.set_xlabel("Número de clientes")
ax1.set_ylabel("Throughput (operaciones/s)")
ax1.grid(True)
ax1.legend(loc="upper left")

ax2 = ax1.twinx()
ax2.plot(range(1, num_clientes), errors, 'o--', label="Errores", color="red", alpha=0.5)
ax2.set_ylabel("% de peticiones con error")
ax2.legend(loc="upper right")

plt.subplot(1, 2, 2)
plt.title(f"Tiempo medio por petición para {num_clientes} clientes concurrentes ({num_comandos_por_cliente} comandos/cliente)")
plt.plot(range(1, num_clientes), medias, 'o-', label="Tiempo/petición", color="orange")
#plt.scatter(range(1, num_clientes), medias, fmt='o-', label="Tiempo/petición", color="orange")
plt.xlabel("Número de clientes")
plt.ylabel("Tiempo medio (s)")
plt.grid()
plt.legend()

plt.tight_layout()
plt.show()
