import random

# Parámetros del problema
N, M = 1000, 1000  # Dimensiones del tablero
K = 70000  # Cantidad de objetivos
L = 50000  # Cantidad de drones

# Generar K objetivos con coordenadas distintas
objetivos_coordenadas = set()
objetivos = []

while len(objetivos) < K:
    x, y = random.randint(0, N-1), random.randint(0, M-1)
    if (x, y) not in objetivos_coordenadas:
        resistencia = random.randint(-10, 100)  # Resistencia arbitraria
        objetivos.append((x, y, resistencia))
        objetivos_coordenadas.add((x, y))

# Generar L drones
drones = []
for _ in range(L):
    x, y = random.randint(0, N-1), random.randint(0, M-1)
    rd = random.randint(1, 10)  # Radio de destrucción
    pe = random.randint(1, 10)  # Poder explosivo
    drones.append((x, y, rd, pe))

# Formatear el input en texto correctamente
input_text = f"{N} {M}\n{K}\n"
input_text += "\n".join(f"{x} {y} {r}" for x, y, r in objetivos) + "\n"
input_text += f"{L}\n"
input_text += "\n".join(f"{x} {y} {rd} {pe}" for x, y, rd, pe in drones)

# Guardar en un archivo
file_path = "/home/yisus/USB/Operativos/Proyecto_2_CI3825/input_grande.txt"
with open(file_path, "w") as file:
    file.write(input_text)

file_path