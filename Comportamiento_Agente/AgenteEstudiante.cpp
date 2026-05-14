/**
 * ============================================================
 *  AgenteEstudiante.cpp — Práctica 3: 5-en-Raya Táctico
 *  Inteligencia Artificial — UGR — Curso 2025-2026
 * ===========================================================
 *
 *  ALGORITMOS:
 *  1. STATUS      — Resolución exhaustiva sin límite de profundidad.
 *  2. MINIMAX     — Árbol de juego con límite de profundidad (comp: 4).
 *  3. ALFA-BETA   — MiniMax con poda (comp: 7 con prof. iterativa).
 *
 *  MEJORAS IMPLEMENTADAS:
 *
 *  A. PROFUNDIDAD ITERATIVA: garantiza movimiento válido ante timeout.
 *
 *  B. EFECTO HORIZONTE: nodos terminales devuelven ±10000000±prof.
 *     Prefiere victorias rápidas y alarga derrotas.
 *
 *  C. MOVE ORDERING: ordena sucesores por heuristicaPrueba (O(81),
 *     muy rápida) más bonus de centralidad del movimiento y prioridad
 *     a casillas verdes. En la raíz aplica Principal Variation Search:
 *     el mejor movimiento de la iteración anterior va primero.
 *
 *  D. HEURÍSTICA 1-2-2-2: 3-en-raya rival = -50000 porque el rival
 *     gana en su PRÓXIMO turno colocando 2 fichas.
 *
 *  E. MODO PÁNICO J2: multiplicador 1.5x defensivo cuando id==2 para
 *     compensar la ventaja de iniciativa del J1 en el sistema 1-2-2-2.
 *
 *  F. CASILLA AMARILLA CALIBRADA: penalización -75000, entre -50000
 *     (3-en-raya) y -100000 (4-en-raya). Solo vale la pena activar
 *     una bomba para romper amenazas de 4, nunca de 3.
 * ============================================================
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

    // Mejora B: efecto horizonte
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
 * JuegaInteligente — Profundidad iterativa + Principal Variation Search.
 * Mejora A: garantiza movimiento válido aunque se agote el tiempo.
 * Mejora C: mejorMovimientoH lleva el hint de la iteración anterior a alfaBeta.
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
 * alfaBeta — MiniMax con poda alfa-beta y mejoras C, B.
 *
 * Mejora C — Move Ordering:
 *   Ordena sucesores por heuristicaPrueba (O(81), muy rápida) más
 *   bonus de centralidad del movimiento y prioridad a casillas verdes.
 *   MAX: mejor primero. MIN: peor para nosotros primero.
 *   En la raíz: PV Search coloca mejorMovimientoH primero.
 *
 * Mejora B — Efecto Horizonte:
 *   Victoria = 10000000 - profundidad (ganar antes vale más).
 *   Derrota  = -10000000 + profundidad (morir tarde vale más).
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

    // Mejora B: efecto horizonte
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool esMax = (tablero.getJugadorTurno() == id);

    // Mejora C: Move Ordering — ordenamos por heuristicaPrueba + bonus centro + verde
    int centro_f = tablero.getFilas()    / 2;
    int centro_c = tablero.getColumnas() / 2;

    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++) {
        double sc = heuristicaPrueba(sucesores[i]); // O(81): rápida
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);
        if (mov.first != -1) {
            // Bonus por centralidad del movimiento concreto
            int dist = std::abs(mov.first - centro_f) + std::abs(mov.second - centro_c);
            sc += (tablero.getFilas() + tablero.getColumnas() - dist) * 2.0;
            // Prioridad absoluta a casillas verdes (dan movimiento extra)
            if (tablero.getTipoCelda(mov.first, mov.second) == Tablero::TipoCelda::VERDE)
                sc += 10000.0;
        }
        ranking.push_back({sc, i});
    }

    if (esMax)
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
    else
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first < b.first; });

    // PV Search: en la raíz, el mejor movimiento de la iteración anterior va primero
    if (profundidad == 0 && mejorMovimientoH.first != -1) {
        for (int i = 1; i < (int)ranking.size(); i++) {
            std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[ranking[i].second]);
            if (mov == mejorMovimientoH) {
                std::rotate(ranking.begin(), ranking.begin() + i, ranking.begin() + i + 1);
                break;
            }
        }
    }

    // Limitamos el branching factor para alcanzar mayor profundidad en el mismo tiempo.
    // Con 12 sucesores en vez de ~25: 12^5=248K nodos vs 25^5=9.7M → profundidad 5 alcanzable.
    // El move ordering garantiza que los 12 elegidos son los más prometedores.
    const int MAX_BRANCH = 12;
    if ((int)ranking.size() > MAX_BRANCH) ranking.resize(MAX_BRANCH);

    if (esMax) {
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
            if (alfa >= beta) break; // PODA BETA
        }
        return mejor;
    } else {
        double mejor = MasInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad+1, prof_Max, alfa, beta, movHijo);
            if (val < mejor) mejor = val;
            beta = std::min(beta, mejor);
            if (beta <= alfa) break; // PODA ALFA
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
    for (int f=0; f< tablero.getFilas(); f++){
        for (int c = 0; c< tablero.getColumnas(); c++){
            if (tablero.getCelda(f,c) != 0){
                int valor = tablero.getFilas()-abs(f-(tablero.getFilas()/2))
                          + tablero.getColumnas()-abs(c-(tablero.getColumnas()/2));
                if (tablero.getCelda(f,c) == id) score_positivo += valor;
                else                             score_negativo += valor;
            }
        }
    }
    return score_positivo - score_negativo;
}

// ============================================================
// Mejora D — evaluarVentana ajustada al modo 1-2-2-2
// Con 3 fichas rivales el rival gana en su PRÓXIMO turno (coloca 2).
// Pesos ofensivos:  50000 / 10000 / 100 / 1
// Pesos defensivos: 100000 / 50000 / 500 / 2
// ============================================================
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
 * heuristica1 — Función principal para la competición (-id1 1).
 *
 * Criterio 1: victoria/derrota inmediata (±10000000, consistente con alfaBeta).
 * Criterio 2: alineaciones parciales en las 4 direcciones (evaluarVentana).
 * Criterio 3: control del centro.
 * Criterio 4: casillas especiales (roja, verde, amarilla).
 * Mejora E:   multiplicador 1.5x defensivo cuando jugamos como J2.
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
                // Mejora E — Modo Pánico J2: peso defensivo 1.5x cuando somos J2
                if (mias == 0 && rival > 0) {
                    double val = evaluarVentana(0, rival, n);
                    score += (id == 2) ? val * 1.5 : val;
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
                // Casilla roja: quien coloca su ficha la pierde (se convierte en del rival).
                if      (celda == id)       score -= 500.0;
                else if (celda == oponente) score += 500.0;

            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Casilla verde: da movimiento extra → bueno si la controlamos.
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;

            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Mejora F: calibrada entre -50000 y -100000.
                // Solo activamos bomba para romper 4-en-raya, nunca 3-en-raya.
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}

/**
 * heuristica2 — Variante ultra-defensiva para comparación en la memoria.
 * Peso defensivo x2 + menor bonus de centro (0.5 vs 1.0).
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
                if      (celda == id)       score -= 500.0;
                else if (celda == oponente) score += 500.0;
            } else if (tipo == Tablero::TipoCelda::VERDE) {
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;
            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}
