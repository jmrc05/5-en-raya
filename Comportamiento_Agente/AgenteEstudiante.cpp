#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>

AgenteEstudiante::AgenteEstudiante(int id, int profundidadMax, double tiempoMax, int numHeuristica, ModoJuego modo) 
    : id(id), profundidadMax(profundidadMax), tiempoMaxSegundos(tiempoMax), numHeuristica(numHeuristica), modo(modo), abortarBanda(false) {
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

    switch (modo)
    {
    case ModoJuego::ALEATORIO:
        return JuegaAleatorio(tablero);
        break;
    case ModoJuego::STATUS:
        Status(tablero, mejor);
        return mejor;
        break;
    case ModoJuego::MINIMAX:
        minimax(tablero, 0, profundidadMax, mejor);
        return mejor;
        break;
    case ModoJuego::INTELIGENTE:
        return JuegaInteligente(tablero);
        break;
    }
    return {-1, -1};
}

/**
 * @brief Compara dos tableros para identificar cuál ha sido el movimiento realizado.
 */
std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero& hijo) {
    for (int f = 0; f < padre.getFilas(); ++f)
        for (int c = 0; c < padre.getColumnas(); ++c)
            if (padre.getCelda(f, c) == 0 && hijo.getCelda(f, c) != 0)
                return {f, c};
    return {-1, -1};
}

/**
 * @brief Agente aleatorio.
 */
std::pair<int, int> AgenteEstudiante::JuegaAleatorio(const Tablero& tablero) {
    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return {-1, -1};
    int elegido = rand() % sucesores.size();
    return SacarMovimiento(tablero, sucesores[elegido]);
}

/**
 * @brief Status — resolución completa sin límite de profundidad.
 * Solo viable en tableros pequeños (3x3, 4x4).
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
        // Nodo MAX: VICTORIA > EMPATE > DERROTA
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
        // Nodo MIN: DERROTA nuestra > EMPATE > VICTORIA nuestra
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
 * @brief Minimax clásico sin poda (profundidad competición: 4).
 */
double AgenteEstudiante::minimax(const Tablero& tablero, int profundidad, int prof_Max, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Nodos terminales con penalización de profundidad (fix efecto horizonte)
    if (ganador == id)       return GANAR - profundidad;
    if (ganador == oponente) return PERDER + profundidad;
    if (ganador == -1)       return 0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    if (tablero.getJugadorTurno() == id) {
        double mejor = MenosInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val > mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    } else {
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val < mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    }
}

/**
 * @brief JuegaInteligente con Profundidad Iterativa y Ordenación entre iteraciones.
 *
 * Mejora 3 — Move ordering entre iteraciones:
 * Guardamos en mejorMovimientoH el mejor movimiento de la iteración d-1.
 * alfaBeta lo colocará como primer candidato en la iteración d.
 * Esto garantiza que la raíz explore primero el movimiento más prometedor,
 * maximizando los cortes alfa-beta desde el principio.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};
    mejorMovimientoH = {-1, -1};  // Reseteamos el hint al inicio de cada turno

    // Seguridad: inicializamos con el primer movimiento legal
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) {
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    } else {
        return mejorMov;
    }

    // Profundidad iterativa de 1 hasta profundidadMax
    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};

        // mejorMovimientoH lleva el hint de la iteración anterior a alfaBeta
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);

        if (abortarBanda) {
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break;
        }
        if (movActual.first != -1) {
            mejorMov         = movActual;
            mejorMovimientoH = movActual;  // hint para la siguiente iteración
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

/**
 * @brief Alfa-Beta con tres mejoras:
 *
 * Mejora 1 — Heurística 1-2-2-2:
 *   Los pesos de evaluarVentana reflejan que cada jugador pone hasta 2 fichas
 *   por turno. Con 3 en línea el rival puede ganar EN EL PRÓXIMO TURNO → -80000.
 *
 * Mejora 2 — Efecto Horizonte:
 *   Los nodos terminales devuelven GANAR-profundidad o PERDER+profundidad.
 *   Así el agente prefiere victorias rápidas y alarga las derrotas.
 *
 * Mejora 3 — Move Ordering entre iteraciones:
 *   En profundidad 0, el mejor movimiento de la iteración anterior
 *   (almacenado en mejorMovimientoH) se coloca primero en la lista de sucesores.
 *   Combinado con la ordenación por heuristicaPrueba, esto maximiza los cortes.
 */
double AgenteEstudiante::alfaBeta(const Tablero& tablero, int profundidad, int prof_Max,
                                   double alfa, double beta, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Mejora 2: nodos terminales con penalización de profundidad.
    // GANAR - profundidad: victorias más rápidas valen más.
    // PERDER + profundidad: derrotas más lentas valen más (el rival puede cometer errores).
    if (ganador == id)       return GANAR - profundidad;
    if (ganador == oponente) return PERDER + profundidad;
    if (ganador == -1)       return 0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool esMax = (tablero.getJugadorTurno() == id);

    // --- Ordenación de sucesores ---
    // Construimos un vector de (puntuacion_rapida, indice) y lo ordenamos.
    // MAX: de mayor a menor (mejor para nosotros primero).
    // MIN: de menor a mayor (peor para nosotros primero).
    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++)
        ranking.push_back({heuristicaPrueba(sucesores[i]), i});

    if (esMax)
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
    else
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first < b.first; });

    // Mejora 3: en la raíz, colocamos primero el mejor movimiento de la iteración anterior.
    // Esto garantiza que alfa-beta empiece por la rama más prometedora,
    // generando cortes desde el inicio y explorando más profundo en el mismo tiempo.
    if (profundidad == 0 && mejorMovimientoH.first != -1) {
        for (int i = 1; i < (int)ranking.size(); i++) {
            std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[ranking[i].second]);
            if (mov == mejorMovimientoH) {
                // Rotamos para que este índice quede en la posición 0
                std::rotate(ranking.begin(), ranking.begin() + i, ranking.begin() + i + 1);
                break;
            }
        }
    }

    if (esMax) {
        // Nodo MAX: maximizamos y actualizamos alfa.
        // Poda BETA: si alfa >= beta, el nodo MIN padre nunca elegiría este camino.
        double mejor = MenosInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
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
        // Nodo MIN: minimizamos y actualizamos beta.
        // Poda ALFA: si beta <= alfa, el nodo MAX abuelo ya tiene algo mejor.
        double mejor = MasInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (val < mejor) mejor = val;
            beta = std::min(beta, mejor);
            if (beta <= alfa) break; // PODA ALFA
        }
        return mejor;
    }
}

/**
 * @brief Dispatcher de heurísticas.
 */
double AgenteEstudiante::heuristica(const Tablero& tablero) {
    switch(numHeuristica) {
        case 0: return heuristicaPrueba(tablero); break;
        case 1: return heuristica1(tablero);      break;
        case 2: return heuristica2(tablero);      break;
        default: return heuristica1(tablero);
    }
}

/**
 * @brief heuristicaPrueba — NO MODIFICAR.
 * Evalúa proximidad de fichas al centro. También usada para ordenar sucesores
 * en alfaBeta por ser O(81) y muy rápida.
 */
double AgenteEstudiante::heuristicaPrueba(const Tablero& tablero) {
    int n = tablero.getNParaGanar();
    int oponente = (id == 1) ? 2 : 1;
    double score_positivo = 0, score_negativo = 0;
    for (int f = 0; f < tablero.getFilas(); f++) {
        for (int c = 0; c < tablero.getColumnas(); c++) {
            if (tablero.getCelda(f, c) != 0) {
                int valor = tablero.getFilas()    - abs(f - (tablero.getFilas()    / 2))
                          + tablero.getColumnas() - abs(c - (tablero.getColumnas() / 2));
                if (tablero.getCelda(f, c) == id) score_positivo += valor;
                else                              score_negativo += valor;
            }
        }
    }
    return score_positivo - score_negativo;
}

// ============================================================
// evaluarVentana — ajustada a la regla 1-2-2-2
//
// En este juego cada jugador coloca HASTA 2 fichas por turno.
// Esto cambia radicalmente el peligro de cada amenaza:
//
//   n-1 fichas propias  (4 de 5): ganamos en ESTE turno        → +100000
//   n-2 fichas propias  (3 de 5): ganamos en el PRÓXIMO turno  → +50000
//   n-3 fichas propias  (2 de 5): necesitamos 2 turnos más     → +500
//   1 ficha propia                                              → +1
//
//   n-1 fichas rivales (4 de 5): rival gana en este turno      → -200000 (BLOQUEAR YA)
//   n-2 fichas rivales (3 de 5): rival gana en el PRÓXIMO turno→ -80000  (casi igual de urgente)
//   n-3 fichas rivales (2 de 5): rival necesita 2 turnos más   → -3000
//   1 ficha rival                                               → -1
//
// Los pesos defensivos son mayores que los ofensivos equivalentes
// porque si no bloqueamos una amenaza inmediata perdemos con certeza.
// ============================================================
static double evaluarVentana(int mias, int rival, int n) {
    if (mias > 0 && rival > 0) return 0.0; // ventana bloqueada, vale 0

    if (mias > 0) {
        if (mias == n - 1) return 100000.0;
        if (mias == n - 2) return  50000.0;
        if (mias == n - 3) return    500.0;
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -200000.0;
        if (rival == n - 2) return  -80000.0;
        if (rival == n - 3) return   -3000.0;
        return -1.0;
    }
    return 0.0;
}

/**
 * @brief heuristica1 — Heurística principal para la competición (-id1 1).
 *
 * Criterio 1 — Victoria/Derrota inmediata.
 * Criterio 2 — Alineaciones parciales (evaluarVentana, ajustada al 1-2-2-2).
 * Criterio 3 — Control del centro.
 * Criterio 4 — Casillas especiales (roja, verde, amarilla).
 */
double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    if (ganador == id)       return  GANAR;
    if (ganador == oponente) return  PERDER;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();
    double score = 0.0;

    // Criterio 2: alineaciones parciales en las 4 direcciones
    const int dfs[] = { 0,  1,  1,  1};
    const int dcs[] = { 1,  0,  1, -1};
    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                int ef = f + df * (n - 1);
                int ec = c + dc * (n - 1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;
                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f + df * k, c + dc * k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }
                score += evaluarVentana(mias, rival, n);
            }
        }
    }

    // Criterio 3: control del centro
    int centro_f = filas / 2, centro_c = cols / 2;
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;
            int dist  = std::abs(f - centro_f) + std::abs(c - centro_c);
            double bonus = (filas + cols - dist) * 0.5;
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
                if      (celda == id)       score -= 500.0;
                else if (celda == oponente) score += 500.0;
            } else if (tipo == Tablero::TipoCelda::VERDE) {
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;
            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Activar una bomba propia destruye nuestro setup → penalización fuerte
                if      (celda == id)       score -= 20000.0;
                else if (celda == oponente) score +=  1000.0;
            }
        }
    }

    return score;
}

/**
 * @brief heuristica2 — Variante ultra-defensiva para comparación en la memoria.
 * Peso defensivo x2 en ventanas + menor bonus de centro.
 */
double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    if (ganador == id)       return  GANAR;
    if (ganador == oponente) return  PERDER;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();
    double score = 0.0;

    const int dfs[] = { 0,  1,  1,  1};
    const int dcs[] = { 1,  0,  1, -1};
    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                int ef = f + df * (n - 1);
                int ec = c + dc * (n - 1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;
                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f + df * k, c + dc * k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }
                if (mias > 0 && rival > 0) continue;
                if      (mias  > 0) score += evaluarVentana(mias,  0, n);
                else if (rival > 0) score += evaluarVentana(0, rival, n) * 2.0;
            }
        }
    }

    int centro_f = filas / 2, centro_c = cols / 2;
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;
            int dist  = std::abs(f - centro_f) + std::abs(c - centro_c);
            double bonus = (filas + cols - dist) * 0.3;
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
                if      (celda == id)       score -= 20000.0;
                else if (celda == oponente) score +=  1000.0;
            }
        }
    }

    return score;
}
