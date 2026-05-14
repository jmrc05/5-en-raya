/**
 * ============================================================
 *  AgenteEstudiante.cpp — Práctica 3: 5-en-Raya Táctico
 *  Inteligencia Artificial — UGR — Curso 2025-2026
 * ============================================================
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
 *  C. MOVE ORDERING ADAPTATIVO:
 *     Ordena sucesores por puntuación táctica rápida, NO solo por centro.
 *     Prioridad: verde > bloqueo de amenaza rival > construcción propia > centro.
 *     Como J2 esto es crítico: bloquear un 4-en-raya rival vale infinito
 *     más que acercarse al centro; el ordering incorrecto agota el tiempo
 *     en movimientos inútiles y aborta la búsqueda en profundidades bajas.
 *     En la raíz: Principal Variation Search (mejor mov. de iteración d-1 va primero).
 *
 *  D. HEURÍSTICA 1-2-2-2: 3-en-raya rival = -50000 porque el rival
 *     gana en su PRÓXIMO turno colocando 2 fichas.
 *
 *  E. MODO PÁNICO J2: multiplicador 1.5x defensivo cuando id==2 para
 *     compensar la ventaja de iniciativa del J1 en el sistema 1-2-2-2.
 *
 *  F. CASILLA AMARILLA CALIBRADA: penalización -75000, entre -50000
 *     (3-en-raya) y -100000 (4-en-raya). Solo vale la pena activar
 *     una bomba para romper amenazas de 4, nunca de 3. Evita el
 *     "doble-bomba" que dejaba al agente sin material.
 *
 *  G. FIX CELDA ROJA (bug corregido): la lógica estaba INVERTIDA.
 *     Celda roja con ficha del rival = nosotros la pusimos y la perdimos.
 *     Celda roja con nuestra ficha = el rival la puso y nos la regaló.
 *
 *  H. CELDA VERDE REVALORIZADA: de 80 a 5000 puntos.
 *     Una celda verde otorga +1 movimiento extra, permitiendo colocar
 *     3 fichas en un turno (en vez de 2). Eso puede transformar una
 *     amenaza de 3 en victoria inmediata. 80 puntos era ridículo.
 *
 *  I. CONCIENCIA DE LA TRINIDAD: si los huecos de una ventana con
 *     3+ fichas rivales son inaccesibles por la regla de Trinidad,
 *     la amenaza es MAYOR (no podemos bloquearla este turno) →
 *     multiplicador 2x sobre el peso defensivo.
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

// ============================================================
// STATUS: resolución completa. Solo viable en tableros pequeños.
// MAX busca VICTORIA > EMPATE > DERROTA.
// MIN busca DERROTA nuestra > EMPATE > VICTORIA nuestra.
// ============================================================
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

// ============================================================
// MINIMAX sin poda. Profundidad máxima competición: 4.
// ============================================================
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

    // Mejora B: efecto horizonte — penalizar la profundidad en terminales
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
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val > mejor) { mejor = val; if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo); }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    } else {
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val < mejor) { mejor = val; if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo); }
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    }
}

// ============================================================
// JUEGA INTELIGENTE: profundidad iterativa + PV Search.
// ============================================================
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};
    mejorMovimientoH = {-1, -1}; // Reseteamos el hint al inicio de cada turno

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
            mejorMovimientoH = movActual; // Hint para la siguiente iteración
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

// ============================================================
// FUNCIÓN AUXILIAR: puntuación táctica rápida para ordenar sucesores.
//
// Mejora C — Move Ordering Adaptativo:
// Prioriza en este orden:
//   1. Casillas VERDES: +10000 (movimiento extra = ventaja enorme)
//   2. Bloqueo de 4-en-raya rival: +8000 (urgente, el rival gana ahora)
//   3. Bloqueo de 3-en-raya rival: +4000 (el rival gana en su turno)
//   4. Extensión de 4-en-raya propio: +3000
//   5. Extensión de 3-en-raya propio: +1000
//   6. Centralidad: -distancia_al_centro
//
// Por qué no usar solo el centro: si el rival tiene 4 en raya en un borde,
// un agente que ordena por centro evaluará primero 20 movimientos inútiles
// antes de llegar al bloqueo crítico. Esto agota el tiempo (abortarBanda)
// y la poda alfa-beta pierde su efectividad.
// ============================================================
static double puntuacionTactica(const Tablero& tablero, const std::pair<int,int>& mov, int id) {
    if (mov.first == -1) return 0.0;

    int oponente = (id == 1) ? 2 : 1;
    int n        = tablero.getNParaGanar();
    int filas    = tablero.getFilas();
    int cols     = tablero.getColumnas();
    double sc    = 0.0;

    // 1. Casilla verde: prioridad máxima
    if (tablero.getTipoCelda(mov.first, mov.second) == Tablero::TipoCelda::VERDE)
        sc += 10000.0;

    // 2-5. Detección de amenazas en las 4 direcciones alrededor del movimiento
    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        // Ventana deslizante centrada en el movimiento
        for (int start = -(n-1); start <= 0; start++) {
            int f0 = mov.first  + dfs[d] * start;
            int c0 = mov.second + dcs[d] * start;
            int fe = f0 + dfs[d] * (n - 1);
            int ce = c0 + dcs[d] * (n - 1);
            if (f0 < 0 || f0 >= filas || c0 < 0 || c0 >= cols) continue;
            if (fe < 0 || fe >= filas || ce < 0 || ce >= cols) continue;

            int mias = 0, rival = 0;
            for (int k = 0; k < n; k++) {
                int ff = f0 + dfs[d]*k, cc = c0 + dcs[d]*k;
                if (ff == mov.first && cc == mov.second) continue; // celda del movimiento
                int celda = tablero.getCelda(ff, cc);
                if      (celda == id)       mias++;
                else if (celda == oponente) rival++;
            }

            // Bloqueo de amenazas rivales (defensivo, máxima prioridad)
            if (mias == 0) {
                if (rival == n - 1) sc += 8000.0; // Bloqueamos 4-en-raya rival
                if (rival == n - 2) sc += 4000.0; // Bloqueamos 3-en-raya rival
            }
            // Extensión de nuestras amenazas (ofensivo)
            if (rival == 0) {
                if (mias == n - 1) sc += 3000.0; // Completamos 4-en-raya
                if (mias == n - 2) sc += 1000.0; // Extendemos 3-en-raya
            }
        }
    }

    // 6. Centralidad como desempate (tablero 9x9, centro en (4,4))
    int dist = std::abs(mov.first - filas/2) + std::abs(mov.second - cols/2);
    sc -= dist;

    return sc;
}

// ============================================================
// ALFA-BETA con poda y todas las mejoras.
// Profundidad máxima en competición: 7 (con profundidad iterativa).
// ============================================================
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

    // --- Mejora C: Move Ordering Adaptativo ---
    // Construimos (puntuación_tactica, índice) y ordenamos.
    // puntuacionTactica prioriza bloqueos y amenazas sobre el centro.
    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++) {
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);
        ranking.push_back({puntuacionTactica(tablero, mov, id), i});
    }

    // MAX: mejor para nosotros primero. MIN: peor para nosotros primero.
    if (esMax)
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
    else
        std::sort(ranking.begin(), ranking.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first < b.first; });

    // Principal Variation Search: en la raíz, el mejor movimiento de la
    // iteración anterior va primero para maximizar cortes desde el inicio.
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
//
// Con 3 fichas rivales, el rival GANA EN SU PRÓXIMO TURNO (coloca 2).
// Pesos ofensivos: 50000 / 10000 / 100 / 1
// Pesos defensivos (siempre mayores): 100000 / 50000 / 500 / 2
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

// ============================================================
// HEURISTICA 1 — Función principal para la competición (-id1 1).
// ============================================================
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

    // Fase actual para la regla de Trinidad (Mejora I)
    int fase = tablero.getFaseActual();

    // Criterio 2: alineaciones parciales en las 4 direcciones
    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                int ef = f + df*(n-1), ec = c + dc*(n-1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;

                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f+df*k, c+dc*k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }

                // Mejora I — Conciencia de la Trinidad:
                // Si la ventana tiene piezas rivales y hay huecos que NO se
                // pueden jugar este turno (Trinity-bloqueados), la amenaza es
                // MAYOR porque no podemos bloquearla. Multiplicamos x2.
                double factor = 1.0;
                if (mias == 0 && rival > 0) {
                    bool hayHuecoBloqueado = false;
                    bool hayHuecoLibre     = false;
                    for (int k = 0; k < n; k++) {
                        int ff = f+df*k, cc = c+dc*k;
                        if (tablero.getCelda(ff, cc) == 0) {
                            // Celda vacía: ¿es jugable este turno?
                            if ((ff + cc) % 3 == fase % 3)
                                hayHuecoLibre = true;
                            else
                                hayHuecoBloqueado = true;
                        }
                    }
                    // Si hay huecos bloqueados y ninguno libre: amenaza imparable
                    if (hayHuecoBloqueado && !hayHuecoLibre)
                        factor = 2.0;
                }

                // Mejora E — Modo Pánico J2: multiplicador 1.5x defensivo
                if (mias == 0 && rival > 0) {
                    double val = evaluarVentana(0, rival, n) * factor;
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
            int dist  = std::abs(f - centro_f) + std::abs(c - centro_c);
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
                // Mejora G — FIX: la lógica estaba INVERTIDA en el código original.
                // La regla dice: quien coloca en rojo pierde esa ficha (se convierte
                // en del rival). Por tanto en el tablero resultante:
                //   - celda == oponente en rojo → NOSOTROS la pusimos y la perdimos → malo
                //   - celda == id en rojo → el RIVAL la puso y nos la regaló → bueno
                if      (celda == oponente) score -= 500.0; // Perdimos una ficha aquí
                else if (celda == id)       score += 500.0; // El rival nos regaló una ficha

            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Mejora H — REVALORIZADA: de 80 a 5000.
                // Una celda verde otorga +1 movimiento extra inmediato, permitiendo
                // colocar 3 fichas en ese turno. Puede transformar una amenaza de 3
                // en victoria inmediata. El valor de 80 era completamente insuficiente.
                if      (celda == id)       score += 5000.0;
                else if (celda == oponente) score -= 5000.0;

            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Mejora F — CALIBRADA: -75000, entre -50000 (3-en-raya) y -100000 (4-en-raya).
                // Solo vale activar una bomba para romper 4-en-raya, nunca para 3-en-raya.
                // Esto evita el "doble-bomba" que dejaba al agente sin material.
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}

// ============================================================
// HEURISTICA 2 — Variante ultra-defensiva para comparación en la memoria.
// Peso defensivo x2 + menor bonus de centro.
// ============================================================
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
