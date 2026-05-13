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
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Efecto horizonte: penalizamos la profundidad en nodos terminales
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
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
 * La profundidad iterativa garantiza que siempre tenemos un movimiento seguro
 * aunque se agote el tiempo (usamos el mejor resultado de la última iteración completa).
 *
 * Mejora 3 — Move Ordering entre iteraciones:
 * mejorMovimientoH guarda el mejor movimiento de la iteración d-1.
 * En la iteración d, alfaBeta lo coloca primero antes de ordenar por heuristicaPrueba.
 * Esto garantiza que la ventana alfa-beta se estreche desde el primer nodo explorado,
 * multiplicando los cortes y permitiendo mayor profundidad en el mismo tiempo.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};
    mejorMovimientoH = {-1, -1};

    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) {
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    } else {
        return mejorMov;
    }

    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);

        if (abortarBanda) {
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break;
        }
        if (movActual.first != -1) {
            mejorMov         = movActual;
            mejorMovimientoH = movActual;
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

/**
 * @brief Alfa-Beta con las tres mejoras implementadas:
 *
 * 1. EFECTO HORIZONTE:
 *    Los nodos terminales devuelven 10000000 - profundidad (victoria) o
 *    -10000000 + profundidad (derrota). Así el agente prefiere victorias
 *    rápidas y alarga las derrotas esperando errores del rival.
 *
 * 2. PESOS 1-2-2-2 (en evaluarVentana):
 *    Con 3 fichas rivales en línea el rival puede ganar en el próximo turno
 *    colocando 2 fichas → -50000. Defensa paranoica.
 *
 * 3. MOVE ORDERING:
 *    - Ordenación por heuristicaPrueba (rápida) en cada nodo.
 *    - En la raíz (profundidad 0): el mejor movimiento de la iteración
 *      anterior (mejorMovimientoH) se coloca primero.
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

    // Mejora 1 — Efecto Horizonte con penalización de profundidad:
    // Ganar rápido vale más (10000000 - 1 < 10000000 - 2 si prof1 < prof2)
    // Perder tarde vale más (-10000000 + 5 > -10000000 + 3 si prof5 > prof3)
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool esMax = (tablero.getJugadorTurno() == id);

    // Move Ordering: ordenamos sucesores por heuristicaPrueba (O(81) por nodo, muy rápida).
    // MAX: mejor para nosotros primero. MIN: peor para nosotros primero.
    // Esto asegura que la ventana alfa-beta se estreche cuanto antes.
    int centro_f = tablero.getFilas()    / 2;
    int centro_c = tablero.getColumnas() / 2;

    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++) {
        // Usamos heuristicaPrueba + bonus de centralidad de la nueva pieza
        double sc = heuristicaPrueba(sucesores[i]);
        // Bonus extra por centralidad del movimiento (la nueva ficha colocada)
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);
        if (mov.first != -1) {
            int dist = std::abs(mov.first - centro_f) + std::abs(mov.second - centro_c);
            sc += (tablero.getFilas() + tablero.getColumnas() - dist) * 2.0;
        }
        ranking.push_back({sc, i});
    }

    if (esMax)
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
    else
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first < b.first; });

    // Move Ordering entre iteraciones: en la raíz colocamos primero el mejor
    // movimiento de la iteración anterior (mejorMovimientoH). Esto garantiza
    // que alfa-beta empiece por la rama más prometedora de la iteración d-1,
    // generando cortes desde el inicio y explorando más profundo en el mismo tiempo.
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
        // Poda BETA: si alfa >= beta, el MIN padre nunca elegiría este camino.
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
        // Poda ALFA: si beta <= alfa, el MAX abuelo ya tiene algo mejor.
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

double AgenteEstudiante::heuristica(const Tablero& tablero) {
    switch(numHeuristica) {
        case 0: return heuristicaPrueba(tablero); break;
        case 1: return heuristica1(tablero);      break;
        case 2: return heuristica2(tablero);      break;
        default: return heuristica1(tablero);
    }
}

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
// Mejora 2 — evaluarVentana ajustada al modo 1-2-2-2
//
// En este juego cada jugador coloca HASTA 2 fichas por turno.
// Esto cambia radicalmente el nivel de peligro de cada amenaza:
//
//   n-1 propias  (4 de 5): ganamos EN ESTE turno      →  50000
//   n-2 propias  (3 de 5): ganamos EN EL PRÓXIMO turno→  10000
//   n-3 propias  (2 de 5): necesitamos 2+ turnos      →    100
//   1 ficha propia                                     →      1
//
//   n-1 rivales (4 de 5): rival gana AHORA            → -100000 (BLOQUEAR YA)
//   n-2 rivales (3 de 5): rival gana EN SU TURNO      →  -50000 (casi igual de urgente)
//   n-3 rivales (2 de 5): rival necesita 2 turnos     →   -500
//   1 ficha rival                                      →     -2
//
// Pesos defensivos mayores que los ofensivos: si no bloqueamos
// una amenaza inmediata, perdemos con certeza.
// ============================================================
static double evaluarVentana(int mias, int rival, int n) {
    if (mias > 0 && rival > 0) return 0.0; // ventana bloqueada

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
 * @brief heuristica1 — Heurística principal para la competición (-id1 1).
 *
 * Criterio 1 — Victoria/Derrota inmediata → ±10000000 (consistente con alfaBeta).
 * Criterio 2 — Alineaciones parciales ajustadas al 1-2-2-2 (evaluarVentana).
 * Criterio 3 — Control del centro.
 * Criterio 4 — Casillas especiales.
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
            double bonus = (filas + cols - dist) * 1.0;
            if (celda == id) score += bonus;
            else             score -= bonus;
        }
    }

    // Criterio 4: casillas especiales
    // Penalización amarilla calibrada entre el peso de 3-en-raya (-50000)
    // y 4-en-raya (-100000): solo vale la pena activar una bomba para
    // romper amenazas de 4, no de 3. Así evitamos activar dos bombas seguidas.
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
                // Calibrado: 50000 < 75000 < 100000
                // Activar bomba cuesta -75000 → vale la pena si rompe 4-en-raya (-100000)
                // pero NO si solo rompe 3-en-raya (-50000)
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
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

    if (ganador == id)       return  10000000.0;
    if (ganador == oponente) return -10000000.0;
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
