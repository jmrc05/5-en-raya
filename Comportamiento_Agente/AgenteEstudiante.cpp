/**
 * @file AgenteEstudiante.cpp
 * @brief Implementación del agente inteligente para el juego 5-en-Raya Táctico (9x9).
 *
 * @details
 * Práctica 3 — Inteligencia Artificial — UGR — Curso 2025-2026
 *
 * Este fichero implementa un agente basado en búsqueda con adversario para
 * el juego 5-en-Raya Táctico. Las reglas especiales condicionan cada decisión:
 *  - Turnos asimétricos 1-2-2-2: J1 coloca 1 ficha en el primer turno;
 *    a partir de ahí ambos colocan hasta 2. Ventaja ofensiva de J1.
 *  - Regla de la Trinidad: solo se puede colocar en (f,c) si
 *    (f+c)%3 == faseActual%3. Crea amenazas matemáticamente imparables.
 *  - Casillas especiales: rojas (ficha va al rival), verdes (movimiento
 *    extra), amarillas (bomba que limpia fila y columna).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * ALGORITMOS
 * ─────────────────────────────────────────────────────────────────────────
 *
 * 1. STATUS — Resolución completa sin límite de profundidad.
 * 2. MINIMAX — Exploración con límite de profundidad (competición: 4).
 * 3. ALFA-BETA — MiniMax con poda (competición: 7 con prof. iterativa).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * MEJORAS IMPLEMENTADAS
 * ─────────────────────────────────────────────────────────────────────────
 *
 * A. PROFUNDIDAD ITERATIVA: garantiza movimiento válido ante timeout.
 *
 * B. EFECTO HORIZONTE: nodos terminales devuelven ±10000000±prof.
 *
 * C. MOVE ORDERING ADAPTATIVO (heuristicaPrueba + detección táctica):
 *    Ordena sucesores combinando heuristicaPrueba (O(81), posicional)
 *    con detección táctica rápida O(4n) que distingue nodos MAX y MIN:
 *      - Nodo MAX (nuestro turno): atacar primero (+8000/+4000),
 *        luego bloquear rival (+6000/+3000).
 *      - Nodo MIN (turno rival): el rival ataca primero (+8000/+4000),
 *        luego bloquea nuestras amenazas (+6000/+3000).
 *    Esta distinción es crítica: como J1 (MAX) explorar primero ataques
 *    genera ventanas amplias pronto; como J2 (MIN) el rival ataca primero
 *    y necesitamos ver esas respuestas para podar correctamente.
 *    PVS: el mejor movimiento de la iteración anterior va primero en la raíz.
 *
 * D. HEURÍSTICA 1-2-2-2: 3-en-raya rival = -50000.
 *
 * E. MODO PÁNICO J2: multiplicador 2.0× defensivo cuando id==2.
 *
 * F. CASILLA AMARILLA CALIBRADA: -75000, entre -50000 y -100000.
 *
 * G. FIX CASILLA ROJA (bug corregido): lógica estaba INVERTIDA.
 *    celda==oponente en rojo → nosotros la pusimos, la perdimos → -500.
 *    celda==id en rojo → el rival la puso, nos la regaló → +500.
 *
 * H. CASILLA VERDE REVALORIZADA: de 80 a 5000 en heurística.
 *
 * I. CONCIENCIA DE LA TRINIDAD: amenazas con huecos Trinity-bloqueados → ×3.
 */

#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>

AgenteEstudiante::AgenteEstudiante(int id, int profundidadMax, double tiempoMax, int numHeuristica, ModoJuego modo)
    : id(id), profundidadMax(profundidadMax), tiempoMaxSegundos(tiempoMax),
      numHeuristica(numHeuristica), modo(modo), abortarBanda(false) {
    nodosVisitados = 0;
}

bool AgenteEstudiante::tieneLimiteDeTiempo() const {
    return modo != ModoJuego::STATUS;
}

std::pair<int, int> AgenteEstudiante::think(const Tablero& tablero) {
    std::pair<int, int> mejor;
    nodosVisitados = 0;
    abortarBanda = false;
    inicioBusqueda = std::chrono::steady_clock::now();
    switch (modo) {
    case ModoJuego::ALEATORIO:   return JuegaAleatorio(tablero);
    case ModoJuego::STATUS:      Status(tablero, mejor); return mejor;
    case ModoJuego::MINIMAX:     minimax(tablero, 0, profundidadMax, mejor); return mejor;
    case ModoJuego::INTELIGENTE: return JuegaInteligente(tablero);
    }
    return {-1, -1};
}

std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero& hijo) {
    for (int f = 0; f < padre.getFilas(); ++f)
        for (int c = 0; c < padre.getColumnas(); ++c)
            if (padre.getCelda(f, c) == 0 && hijo.getCelda(f, c) != 0)
                return {f, c};
    return {-1, -1};
}

std::pair<int, int> AgenteEstudiante::JuegaAleatorio(const Tablero& tablero) {
    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return {-1, -1};
    int elegido = rand() % sucesores.size();
    return SacarMovimiento(tablero, sucesores[elegido]);
}

/**
 * @brief STATUS: resolución completa. Nodo MAX: VICTORIA>EMPATE>DERROTA.
 *        Nodo MIN: DERROTA nuestra > EMPATE > VICTORIA nuestra.
 */
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero& tablero, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();
    if (ganador == id)       return Resultado::VICTORIA;
    if (ganador == oponente) return Resultado::DERROTA;
    if (ganador == -1)       return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return Resultado::EMPATE;

    bool hayEmpate = false;
    std::pair<int,int> movEmpate = {-1, -1};

    if (tablero.getJugadorTurno() == id) {
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            Resultado r = Status(hijo, movHijo);
            std::pair<int,int> mov = SacarMovimiento(tablero, hijo);
            if (r == Resultado::VICTORIA) { Mov = mov; return Resultado::VICTORIA; }
            if (r == Resultado::EMPATE && !hayEmpate) { hayEmpate = true; movEmpate = mov; }
        }
        if (hayEmpate) { Mov = movEmpate; return Resultado::EMPATE; }
        Mov = SacarMovimiento(tablero, sucesores[0]);
        return Resultado::DERROTA;
    } else {
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            Resultado r = Status(hijo, movHijo);
            if (r == Resultado::DERROTA) return Resultado::DERROTA;
            if (r == Resultado::EMPATE)  hayEmpate = true;
        }
        if (hayEmpate) return Resultado::EMPATE;
        return Resultado::VICTORIA;
    }
}

/**
 * @brief MINIMAX sin poda. Profundidad competición: 4.
 *        Mejora B: efecto horizonte con ±10000000±profundidad.
 */
double AgenteEstudiante::minimax(const Tablero& tablero, int profundidad, int prof_Max, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true; return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Mejora B — Efecto Horizonte
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    if (tablero.getJugadorTurno() == id) {
        double mejor = MenosInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad+1, prof_Max, movHijo);
            if (val > mejor) { mejor = val; if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo); }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    } else {
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad+1, prof_Max, movHijo);
            if (val < mejor) { mejor = val; if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo); }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    }
}

/**
 * @brief Profundidad Iterativa + PVS (Mejoras A y C).
 *        Garantiza movimiento válido ante timeout.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};
    mejorMovimientoH = {-1, -1};

    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty())
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    else
        return mejorMov;

    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);
        if (abortarBanda) {
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break;
        }
        if (movActual.first != -1) {
            mejorMov = movActual;
            mejorMovimientoH = movActual;
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

/**
 * @brief ALFA-BETA con poda. Profundidad competición: 7. Mejoras B y C.
 *
 * @details
 * Mejora B — Efecto Horizonte: ±10000000±profundidad en terminales.
 *
 * Mejora C — Move Ordering Adaptativo:
 *   Combina heuristicaPrueba (O(81), posicional) con detección táctica
 *   rápida O(4n) que distingue nodos MAX y MIN:
 *     - En MAX (nuestro turno): nuestras amenazas (+8000/+4000) tienen
 *       más prioridad que los bloques defensivos (+6000/+3000).
 *       Explorar primero nuestros ataques genera alfa altas rápidamente
 *       y poda más ramas del subárbol MIN.
 *     - En MIN (turno rival): las amenazas del rival (+8000/+4000)
 *       tienen prioridad sobre bloquear las nuestras (+6000/+3000).
 *       El rival elige sus mejores movimientos primero, lo que genera
 *       betas bajas rápidamente y poda más ramas del subárbol MAX.
 *   Esta distinción MAX/MIN es la corrección clave sobre versiones
 *   anteriores donde el ordering era el mismo para ambos nodos.
 *
 *   PVS: en la raíz, el mejor movimiento de la iteración anterior
 *   (mejorMovimientoH) se coloca primero para maximizar los cortes.
 *
 * @param tablero Estado actual.
 * @param profundidad Nivel actual (0=raíz).
 * @param prof_Max Límite de profundidad.
 * @param alfa Cota inferior para MAX. @param beta Cota superior para MIN.
 * @param Mov Jugada elegida (solo actualizada en profundidad 0).
 * @return Valor heurístico del estado tras la poda.
 */
double AgenteEstudiante::alfaBeta(const Tablero& tablero, int profundidad, int prof_Max,
                                   double alfa, double beta, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true; return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Mejora B — Efecto Horizonte
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool esMax = (tablero.getJugadorTurno() == id);

    // Mejora C — Move Ordering Adaptativo con detección táctica MAX/MIN
    int centro_f = tablero.getFilas()    / 2;
    int centro_c = tablero.getColumnas() / 2;
    int n        = tablero.getNParaGanar();

    // Direcciones: horizontal, vertical, diagonal \, diagonal /
    const int dfs2[] = {0, 1, 1, 1};
    const int dcs2[] = {1, 0, 1, -1};

    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());

    for (int i = 0; i < (int)sucesores.size(); i++) {
        // Base: heuristicaPrueba evalúa la posición global resultante (O(81))
        double sc = heuristicaPrueba(sucesores[i]);
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);

        if (mov.first != -1) {
            // Centralidad: casillas centrales tienen más conexiones posibles
            int dist = std::abs(mov.first - centro_f) + std::abs(mov.second - centro_c);
            sc += (tablero.getFilas() + tablero.getColumnas() - dist) * 2.0;

            // Verde: bonus moderado (valioso pero no debe dominar sobre bloques urgentes)
            if (tablero.getTipoCelda(mov.first, mov.second) == Tablero::TipoCelda::VERDE)
                sc += 500.0;

            // Detección táctica rápida O(4n): contamos piezas consecutivas
            // desde el movimiento en ambas direcciones de cada eje
            for (int d = 0; d < 4; d++) {
                int piezasRival = 0, piezasMias = 0;

                for (int dir = -1; dir <= 1; dir += 2) {
                    for (int k = 1; k < n; k++) {
                        int ff = mov.first  + dfs2[d] * dir * k;
                        int cc = mov.second + dcs2[d] * dir * k;
                        if (ff < 0 || ff >= tablero.getFilas() ||
                            cc < 0 || cc >= tablero.getColumnas()) break;
                        int celda = tablero.getCelda(ff, cc);
                        if      (celda == oponente) piezasRival++;
                        else if (celda == id)       piezasMias++;
                        else break;  // hueco vacío interrumpe la cadena
                    }
                }

                if (esMax) {
                    // Nodo MAX (nuestro turno): priorizamos atacar primero.
                    // Explorar primero nuestras amenazas genera alfas altas
                    // rápidamente, podando más ramas del subárbol MIN.
                    if      (piezasMias >= n-1) sc += 8000.0;  // Completamos 4-en-raya
                    else if (piezasMias >= n-2) sc += 4000.0;  // Completamos 3-en-raya
                    if      (piezasRival >= n-1) sc += 6000.0; // Bloqueamos 4-en-raya rival
                    else if (piezasRival >= n-2) sc += 3000.0; // Bloqueamos 3-en-raya rival
                } else {
                    // Nodo MIN (turno rival): el rival ataca primero.
                    // Explorar primero los ataques del rival genera betas bajas
                    // rápidamente, podando más ramas del subárbol MAX.
                    if      (piezasRival >= n-1) sc += 8000.0; // Rival completa 4-en-raya
                    else if (piezasRival >= n-2) sc += 4000.0; // Rival completa 3-en-raya
                    if      (piezasMias >= n-1)  sc += 6000.0; // Rival bloquea nuestro 4-en-raya
                    else if (piezasMias >= n-2)  sc += 3000.0; // Rival bloquea nuestro 3-en-raya
                }
            }
        }
        ranking.push_back({sc, i});
    }

    // MAX: mayor puntuación primero. MIN: menor puntuación primero.
    if (esMax)
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){
                      return a.first > b.first;
                  });
    else
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){
                      return a.first < b.first;
                  });

    // PVS: en la raíz colocamos el mejor movimiento de la iteración anterior
    if (profundidad == 0 && mejorMovimientoH.first != -1) {
        for (int i = 1; i < (int)ranking.size(); i++) {
            std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[ranking[i].second]);
            if (mov == mejorMovimientoH) {
                std::rotate(ranking.begin(), ranking.begin() + i, ranking.begin() + i + 1);
                break;
            }
        }
    }

    if (esMax) {
        // Nodo MAX: maximizamos y actualizamos alfa.
        // Poda BETA: alfa >= beta → MIN padre nunca elegiría este camino.
        double mejor = MenosInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad+1, prof_Max, alfa, beta, movHijo);
            if (val > mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
            alfa = std::max(alfa, mejor);
            if (alfa >= beta) break;  // PODA BETA
        }
        return mejor;
    } else {
        // Nodo MIN: minimizamos y actualizamos beta.
        // Poda ALFA: beta <= alfa → MAX abuelo ya tiene algo mejor.
        double mejor = MasInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad+1, prof_Max, alfa, beta, movHijo);
            if (val < mejor) mejor = val;
            beta = std::min(beta, mejor);
            if (beta <= alfa) break;  // PODA ALFA
        }
        return mejor;
    }
}

double AgenteEstudiante::heuristica(const Tablero& tablero) {
    switch(numHeuristica) {
        case 0:  return heuristicaPrueba(tablero);
        case 1:  return heuristica1(tablero);
        case 2:  return heuristica2(tablero);
        default: return heuristica1(tablero);
    }
}

double AgenteEstudiante::heuristicaPrueba(const Tablero& tablero) {
    // n es el número de fichas en línea para ganar.
    int n = tablero.getNParaGanar();
    int oponente = (id == 1) ? 2 : 1;
    double score_positivo = 0;
    double score_negativo = 0;

    for (int f=0; f< tablero.getFilas(); f++ ){
        for (int c = 0; c< tablero.getColumnas(); c++){
            if (tablero.getCelda(f,c) != 0 ){
                int valor = tablero.getFilas()-abs(f-(tablero.getFilas()/2)) + tablero.getColumnas()-abs(c-(tablero.getColumnas()/2));
                if (tablero.getCelda(f,c) == id){
                  score_positivo += valor;
                 }
                else {
                  score_negativo += valor;
                }
            }
        }
    }
    return score_positivo - score_negativo;
}

/**
 * @brief Evalúa una ventana de n casillas (Mejora D, ajustada al 1-2-2-2).
 * @details Con 3 fichas rivales el rival gana en su próximo turno (coloca 2).
 *          Pesos ofensivos: 50000/10000/100/1.
 *          Pesos defensivos: 100000/50000/500/2 (siempre mayores).
 */
static double evaluarVentana(int mias, int rival, int n) {
    if (mias > 0 && rival > 0) return 0.0;
    if (mias > 0) {
        if (mias == n - 1) return  50000.0;
        if (mias == n - 2) return  10000.0;
        if (mias == n - 3) return    100.0;
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -100000.0;
        if (rival == n - 2) return  -50000.0;
        if (rival == n - 3) return    -500.0;
        return -2.0;
    }
    return 0.0;
}

/**
 * @brief Heurística principal para competición (-id1 1). Implementa Mejoras D-I.
 *
 * @details
 * Criterio 1 — Victoria/Derrota inmediata (±10000000).
 *
 * Criterio 2 — Alineaciones parciales (Mejora D + E + I):
 *   Evalúa ventanas de tamaño n en las 4 direcciones con evaluarVentana.
 *   Mejora E: multiplicador 2.0× defensivo cuando id==2 (Modo Pánico J2).
 *   Mejora I: si todos los huecos de una ventana con ≥3 fichas rivales
 *   son Trinity-bloqueados este turno → amenaza imparable → ×3.
 *
 * Criterio 3 — Control del centro.
 *
 * Criterio 4 — Casillas especiales:
 *   Mejora G (Fix roja): celda==oponente → -500; celda==id → +500.
 *   Mejora H (Verde revalorizada): 80 → 5000.
 *   Mejora F (Amarilla calibrada): -75000.
 *
 * @param tablero Estado a evaluar.
 * @return Puntuación (positiva=ventaja propia, negativa=ventaja rival).
 */
double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    if (ganador == id)       return  10000000.0;
    if (ganador == oponente) return -10000000.0;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();
    double score = 0.0;

    // Fase actual para Mejora I (Conciencia de la Trinidad)
    int fase = tablero.getFaseActual();

    // Criterio 2: alineaciones parciales en las 4 direcciones
    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                int ef = f+df*(n-1), ec = c+dc*(n-1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;

                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f+df*k, c+dc*k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }

                if (mias == 0 && rival > 0) {
                    // Mejora I — Conciencia de la Trinidad:
                    // Si ≥3 fichas rivales y todos los huecos son Trinity-bloqueados
                    // este turno, la amenaza es matemáticamente imparable → ×3.
                    double factorTrinidad = 1.0;
                    if (rival >= n - 2) {
                        bool hayHuecoAccesible = false;
                        for (int k = 0; k < n; k++) {
                            int ff = f+df*k, cc = c+dc*k;
                            if (tablero.getCelda(ff, cc) == 0) {
                                if ((ff + cc) % 3 == fase % 3) {
                                    hayHuecoAccesible = true;
                                    break;
                                }
                            }
                        }
                        if (!hayHuecoAccesible) factorTrinidad = 3.0;
                    }

                    double val = evaluarVentana(0, rival, n) * factorTrinidad;
                    // Mejora E — Modo Pánico J2: ×2.0 defensivo cuando somos J2
                    score += (id == 2) ? val * 2.0 : val;
                } else {
                    score += evaluarVentana(mias, rival, n);
                }
            }
        }
    }

    // Criterio 3: control del centro
    int centro_f = filas/2, centro_c = cols/2;
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;
            int dist  = std::abs(f-centro_f) + std::abs(c-centro_c);
            double bonus = (filas + cols - dist) * 1.0;
            if (celda == id) score += bonus;
            else             score -= bonus;
        }
    }

    // Criterio 4: casillas especiales
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            Tablero::TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == Tablero::TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);

            if (tipo == Tablero::TipoCelda::ROJO) {
                // Mejora G — Fix (lógica estaba INVERTIDA):
                // Quien coloca en rojo pierde la ficha (se convierte en del rival).
                // celda==oponente → nosotros la pusimos, la perdimos → penalizar
                // celda==id       → rival la puso, nos la regaló    → bonificar
                if      (celda == oponente) score -= 500.0;
                else if (celda == id)       score += 500.0;

            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Mejora H — Revalorizada de 80 a 5000:
                // +1 movimiento extra = hasta 3 fichas/turno en 1-2-2-2
                if      (celda == id)       score += 5000.0;
                else if (celda == oponente) score -= 5000.0;

            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Mejora F — Calibrada entre -50000 y -100000:
                // Solo activar bombas para romper 4-en-raya, nunca 3-en-raya
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}

/**
 * @brief Heurística alternativa ultra-defensiva para comparación en la memoria.
 * @details Peso defensivo ×2 en ventanas, centro 0.5, sin Trinidad.
 *          Incluye fixes G y H para comparación justa.
 */
double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    if (ganador == id)       return  10000000.0;
    if (ganador == oponente) return -10000000.0;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();
    double score = 0.0;

    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};
    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                int ef = f+df*(n-1), ec = c+dc*(n-1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;
                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f+df*k, c+dc*k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }
                if (mias > 0 && rival > 0) continue;
                if      (mias  > 0) score += evaluarVentana(mias, 0, n);
                else if (rival > 0) score += evaluarVentana(0, rival, n) * 2.0;
            }
        }
    }

    int centro_f = filas/2, centro_c = cols/2;
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;
            int dist  = std::abs(f-centro_f) + std::abs(c-centro_c);
            double bonus = (filas + cols - dist) * 0.5;
            if (celda == id) score += bonus;
            else             score -= bonus;
        }
    }

    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            Tablero::TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == Tablero::TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);
            if (tipo == Tablero::TipoCelda::ROJO) {
                if      (celda == oponente) score -= 500.0;
                else if (celda == id)       score += 500.0;
            } else if (tipo == Tablero::TipoCelda::VERDE) {
                if      (celda == id)       score += 5000.0;
                else if (celda == oponente) score -= 5000.0;
            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}
