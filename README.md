# 🎮 5-en-Raya Táctico — Agente Inteligente con Poda Alfa-Beta

> Práctica 3 · Inteligencia Artificial · Universidad de Granada (UGR) · Curso 2025-2026

---

## 📋 Descripción

Implementación de un **agente inteligente** capaz de jugar al **5-en-Raya Táctico** en un tablero 9×9 con reglas asimétricas y casillas especiales, desarrollada como parte de la asignatura de Inteligencia Artificial del Grado en Ingeniería Informática de la UGR.

El agente utiliza técnicas de **Búsqueda con Adversario** para tomar decisiones óptimas en tiempo real, compitiendo contra agentes pre-entrenados de distintos niveles de dificultad (*ninjas*).

---

## 🧠 Algoritmos Implementados

| Algoritmo | Descripción |
|-----------|-------------|
| **Status** | Búsqueda exhaustiva para resolución exacta de tableros pequeños |
| **MiniMax** | Exploración del árbol de juego con profundidad limitada |
| **Poda Alfa-Beta** | Optimización de MiniMax eliminando ramas no prometedoras |
| **Heurística propia** | Función de evaluación diseñada para el 5-en-Raya Táctico |

---

## 🕹️ El Juego: 5-en-Raya Táctico (Modo Competición 9×9)

A diferencia del tres en raya clásico, esta variante introduce mecánicas estratégicas avanzadas:

- **Objetivo:** Alinear 5 fichas consecutivas (horizontal, vertical o diagonal)
- **Turno asimétrico (1-2-2-2):** El J1 coloca 1 ficha en el primer turno; a partir de ahí, cada jugador coloca hasta 2 fichas por turno
- **Regla de Trinidad:** Solo se puede colocar en casillas donde `(fila + columna) % 3 == faseActual % 3`
- **Regla de Adyacencia:** Toda pieza debe colocarse adyacente a una ya existente

### Casillas Especiales

| Casilla | Efecto |
|---------|--------|
| 🟢 **Verde (Mística)** | Otorga +1 movimiento extra al turno |
| 🔴 **Roja (Sabotaje)** | La ficha colocada se convierte en del rival |
| 🟡 **Amarilla (Bomba)** | Elimina todas las fichas de su fila y columna |

---

## ⚙️ Instalación y Ejecución

### Requisitos
- Ubuntu / Linux
- OpenGL, CMake, g++

### Instalación
```bash
git clone https://github.com/jmrc05/Practica-3.git
cd Practica-3
./install.sh
```

### Compilar tras modificaciones
```bash
make -j
```

### Ejemplos de ejecución

```bash
# Agente inteligente (heurística 1) vs Ninja nivel 1
./n_en_raya -p1 inteligente -id1 1 -p2 ninja1 -nogui

# Comparativa de heurísticas
./n_en_raya -p1 inteligente -id1 1 -p2 inteligente -id2 0 -nogui

# Partida con interfaz gráfica
./n_en_raya -p1 inteligente -id1 1 -p2 ninja2 -f 9 -c 9 -n 5

# Resolución completa de tablero 3x3 (Status)
./n_en_raya -p1 status -f 3 -c 3 -n 3 -nogui
```

---

## 📁 Estructura del Proyecto

```
Practica-3/
├── src/
│   └── AgenteEstudiante.cpp   # ← Implementación del agente (fichero principal)
├── include/
│   └── AgenteEstudiante.hpp   # ← Cabecera del agente
├── n_en_raya                  # Ejecutable con interfaz gráfica
├── n_en_rayaSH                # Ejecutable sin hebras (más compatible)
└── install.sh                 # Script de instalación
```

---

## 🏆 Resultados en Competición

El agente compite contra 4 niveles de ninjas pre-entrenados. Resultados obtenidos:

| Rival | Resultado |
|-------|-----------|
| ninja1 | ✅ Victoria |
| ninja2 | ✅ Victoria |
| ninja3 | 🔄 En progreso |
| ninja4 | 🔄 En progreso |

---

## 🔬 Diseño de la Heurística

La función de evaluación (`heuristica1`) considera los siguientes criterios, ordenados por importancia:

1. **Victoria/Derrota inmediata** → ±∞
2. **Alineaciones parciales** → evaluación de ventanas de 5 casillas en las 4 direcciones
3. **Casillas especiales** → bonus por verdes, penalización por rojas/amarillas
4. **Control del centro** → bonus por fichas en posiciones centrales del tablero
5. **Amenazas del rival** → evaluación defensiva con signo negativo

---

## 🛠️ Tecnologías

![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-GLUT-5586A4?style=flat&logo=opengl)
![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C?style=flat&logo=cmake)
![Ubuntu](https://img.shields.io/badge/Ubuntu-Linux-E95420?style=flat&logo=ubuntu&logoColor=white)

---

## 📚 Contexto Académico

- **Asignatura:** Inteligencia Artificial
- **Titulación:** Grado en Ingeniería Informática / Telecomunicación
- **Universidad:** Universidad de Granada (UGR)
- **Departamento:** Ciencias de la Computación e Inteligencia Artificial (DECSAI)
- **Curso:** 2025-2026

---

## 👤 Autor

**Juana Maria Rascon Contreras** — [@jmrc05](https://github.com/jmrc05)

*Estudiante de Ingeniería Informática · UGR*

---

> ⚠️ **Nota:** Este repositorio es privado y corresponde a trabajo académico individual. El código es de elaboración propia siguiendo las directrices de la asignatura.
