/**
 * ============================================================
 *  AgenteEstudiante.cpp — Práctica 3: 5-en-Raya Táctico
 *  Inteligencia Artificial — UGR — Curso 2025-2026
 * ============================================================
 *
 *  DESCRIPCIÓN GENERAL:
 *  Implementación de un agente inteligente para el juego 5-en-Raya
 *  Táctico (tablero 9x9) usando Búsqueda con Adversario.
 *
 *  ALGORITMOS IMPLEMENTADOS:
 *
 *  1. STATUS — Resolución completa sin límite de profundidad.
 *     Determina matemáticamente si una posición es VICTORIA, DERROTA
 *     o EMPATE si ambos jugadores juegan de forma óptima. Solo viable
 *     en tableros pequeños (3x3, 4x4) por su coste exponencial.
 *
 *  2. MINIMAX — Exploración del árbol de juego con límite de profundidad.
 *     En cada nodo MAX (nuestro turno) elegimos el hijo con mayor valor;
 *     en nodo MIN (turno rival) elegimos el de menor valor.
 *     Profundidad máxima en competición: 4.
 *
 *  3. ALFA-BETA — Mejora de MiniMax que elimina ramas innecesarias.
 *     Mantiene dos cotas: alfa (lo mejor para MAX) y beta (lo mejor
 *     para MIN). Si alfa >= beta en un nodo MAX, el MIN padre nunca
 *     elegiría ese camino → cortamos (poda BETA). Viceversa en MIN
 *     (poda ALFA). Permite llegar a profundidad 7 en el mismo tiempo
 *     que MiniMax puro llega a profundidad 4.
 *     Profundidad máxima en competición: 7 (con profundidad iterativa).
 *
 *  MEJORAS IMPLEMENTADAS:
 *
 *  A. PROFUNDIDAD ITERATIVA (en JuegaInteligente):
 *     Ejecuta alfa-beta de profundidad 1 a profundidadMax de forma
 *     incremental. Si el tiempo se agota, siempre tenemos el mejor
 *     movimiento de la última iteración COMPLETA. Evita devolver
 *     movimientos basados en búsquedas incompletas.
 *
 *  B. EFECTO HORIZONTE (en alfaBeta):
 *     Los nodos terminales devuelven 10000000 - profundidad (victoria)
 *     o -10000000 + profundidad (derrota). Así el agente prefiere
 *     victorias rápidas y alarga las derrotas, en vez de tratarlas
 *     todas como igualmente buenas/malas.
 *
 *  C. MOVE ORDERING — ORDENACIÓN DE MOVIMIENTOS (en alfaBeta):
 *     Antes de explorar los hijos, los ordenamos de mejor a peor
 *     usando tres criterios de prioridad:
 *       1. Casillas verdes primero (dan movimiento extra → ventaja enorme)
 *       2. Proximidad al centro (casillas centrales tienen más conexiones)
 *       3. Mejor movimiento de la iteración anterior (mejorMovimientoH)
 *          colocado primero en la raíz → principal variation search.
 *     Un buen orden hace que alfa-beta pode ramas desde el inicio,
 *     reduciendo el árbol de O(b^d) a O(b^(d/2)) en el mejor caso.
 *
 *  D. HEURÍSTICA AJUSTADA AL 1-2-2-2 (evaluarVentana):
 *     Cada jugador coloca HASTA 2 fichas por turno. Esto significa que
 *     con 3 fichas en línea, el rival puede completar 5 en el PRÓXIMO
 *     turno (colocando 2). Por tanto, 3-en-raya rival vale -50000,
 *     casi tan urgente como 4-en-raya (-100000).
 *
 *  E. MODO PÁNICO J2 (en heuristica1):
 *     Cuando jugamos como J2, el ninja tiene la iniciativa del primer
 *     turno (ventaja matemática del 1-2-2-2). Para compensar, aplicamos
 *     un multiplicador 1.5x a todos los pesos defensivos cuando id==2.
 *
 *  F. CALIBRACIÓN CASILLA AMARILLA:
 *     Penalización -75000: entre -50000 (3-en-raya) y -100000 (4-en-raya).
 *     El agente solo activará una bomba para romper amenazas de 4,
 *     nunca para 3-en-raya. Evita el doble-bomba que dejaba al agente
 *     sin material y en posición perdedora.
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

// ============================================================
// STATUS — Búsqueda exhaustiva sin límite de profundidad.
// En nodos MAX buscamos VICTORIA > EMPATE > DERROTA.
// En nodos MIN el rival busca DERROTA nuestra > EMPATE > VICTORIA nuestra.
// ============================================================
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero& tablero, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Casos base: alguien ganó o empate técnico (tablero lleno)
    if (ganador == id)       return Resultado::VICTORIA;
    if (ganador == oponente) return Resultado::DERROTA;
    if (ganador == -1)       return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return Resultado::EMPATE;

    bool hayEmpate = false;
    std::pair<int,int> movEmpate = {-1, -1};

    if (tablero.getJugadorTurno() == id) {
        // Nodo MAX: buscamos el mejor resultado posible para nosotros
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
        // Nodo MIN: el rival busca el peor resultado para nosotros
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
// MINIMAX — Profundidad máxima en competición: 4.
// Sin poda, explora todo el árbol hasta prof_Max.
// ============================================================
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

    // Mejora B — Efecto Horizonte: penalizamos la profundidad en nodos terminales
    // Ganar antes vale más; perder más tarde vale más (el rival puede equivocarse)
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    if (tablero.getJugadorTurno() == id) {
        // Nodo MAX: elegimos el hijo con mayor valor heurístico
        double mejor = MenosInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val > mejor) {
                mejor = val;
                // Solo guardamos el movimiento en la raíz (profundidad 0)
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Seguridad: garantizamos que siempre haya un movimiento guardado
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    } else {
        // Nodo MIN: elegimos el hijo con menor valor heurístico
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

// ============================================================
// JUEGA INTELIGENTE — Profundidad Iterativa + Move Ordering entre iteraciones.
//
// Mejora A — Profundidad Iterativa:
//   Ejecuta alfaBeta de profundidad 1 hasta profundidadMax de forma
//   incremental. Si el tiempo se agota, devolvemos el mejor movimiento
//   de la última iteración COMPLETA (mejorMovimientoH), nunca uno basado
//   en una búsqueda incompleta.
//
// Mejora C — Move Ordering entre iteraciones (Principal Variation Search):
//   mejorMovimientoH guarda el mejor movimiento de la iteración d-1.
//   alfaBeta lo coloca primero en la iteración d, maximizando los cortes
//   desde el principio del árbol.
// ============================================================
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};

    // Reseteamos el hint de la iteración anterior al inicio de cada turno
    mejorMovimientoH = {-1, -1};

    // Seguridad: inicializamos con el primer movimiento legal por si el tiempo
    // se agota antes de completar profundidad 1
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) {
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    } else {
        return mejorMov; // Sin movimientos posibles
    }

    // Bucle de profundidad iterativa: de 1 hasta profundidadMax
    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};

        // alfaBeta usa mejorMovimientoH como hint para la ordenación en la raíz
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);

        if (abortarBanda) {
            // Tiempo agotado: descartamos esta iteración incompleta
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break;
        }
        if (movActual.first != -1) {
            mejorMov         = movActual;
            mejorMovimientoH = movActual; // Guardamos como hint para la siguiente iteración
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

// ============================================================
// ALFA-BETA — Profundidad máxima en competición: 7.
//
// Mejora B — Efecto Horizonte:
//   Victoria = 10000000 - profundidad (ganar antes es mejor)
//   Derrota  = -10000000 + profundidad (morir más tarde es mejor)
//
// Mejora C — Move Ordering en cada nodo:
//   Ordenamos los sucesores antes de explorarlos con tres criterios:
//   1. Casillas VERDES primero: dan movimiento extra, ventaja enorme
//   2. Centralidad: casillas más cercanas al centro (4,4) tienen más
//      conexiones posibles y generalmente son mejores
//   3. En la raíz (profundidad 0): el mejor movimiento de la iteración
//      anterior (mejorMovimientoH) va primero → Principal Variation Search
//   Un buen orden hace que la primera rama explorada sea la mejor,
//   lo que estrecha la ventana alfa-beta desde el inicio y genera
//   muchos más cortes, permitiendo mayor profundidad en el mismo tiempo.
//
// Poda BETA (en MAX): si alfa >= beta, el MIN padre nunca elegiría aquí
// Poda ALFA (en MIN): si beta <= alfa, el MAX abuelo ya tiene algo mejor
// ============================================================
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

    // Mejora B — Efecto Horizonte con penalización de profundidad
    if (ganador == id)       return  10000000.0 - profundidad;
    if (ganador == oponente) return -10000000.0 + profundidad;
    if (ganador == -1)       return  0.0;
    if (profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool esMax = (tablero.getJugadorTurno() == id);

    // --- Mejora C: MOVE ORDERING ---
    // Construimos un vector de (puntuación_rápida, índice) para ordenar
    // sin mover los tableros (evitamos copias innecesarias).
    // La puntuación combina centralidad + prioridad a casillas verdes.
    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());

    for (int i = 0; i < (int)sucesores.size(); i++) {
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);
        double sc = 0.0;

        if (mov.first != -1) {
            // Criterio 1: Casillas verdes → prioridad ABSOLUTA
            // Una casilla verde da un movimiento extra, equivale a ganar
            // casi medio turno más. Merece ser explorada siempre primero.
            if (tablero.getTipoCelda(mov.first, mov.second) == Tablero::TipoCelda::VERDE)
                sc += 100.0;

            // Criterio 2: Proximidad al centro (4,4) en tablero 9x9
            // Las casillas centrales tienen más ventanas de 5 pasando por ellas
            // y por tanto más potencial ofensivo y defensivo.
            int dist = std::abs(mov.first - 4) + std::abs(mov.second - 4);
            sc -= dist; // Menor distancia → mayor puntuación
        }

        ranking.push_back({sc, i});
    }

    // MAX ordena de mayor a menor (exploramos el mejor movimiento primero)
    // MIN ordena de menor a mayor (exploramos el peor para nosotros primero)
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

    // Criterio 3 (solo en la raíz): Principal Variation Search.
    // Colocamos el mejor movimiento de la iteración anterior (mejorMovimientoH)
    // en la primera posición. Así alfa-beta empieza por el camino más prometedor
    // según la búsqueda de profundidad d-1, generando la mejor ventana posible
    // desde el nodo raíz y maximizando los cortes en toda la iteración actual.
    if (profundidad == 0 && mejorMovimientoH.first != -1) {
        for (int i = 1; i < (int)ranking.size(); i++) {
            std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[ranking[i].second]);
            if (mov == mejorMovimientoH) {
                // Rotamos para que el hint quede en la posición 0
                std::rotate(ranking.begin(), ranking.begin() + i, ranking.begin() + i + 1);
                break;
            }
        }
    }
    // --- Fin Move Ordering ---

    if (esMax) {
        // Nodo MAX: maximizamos y actualizamos alfa
        double mejor = MenosInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (val > mejor) {
                mejor = val;
                // Guardamos el movimiento solo en la raíz del árbol
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Garantizamos que siempre haya un movimiento guardado en la raíz
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
            alfa = std::max(alfa, mejor);
            if (alfa >= beta) break; // PODA BETA: el MIN padre no elegiría aquí
        }
        return mejor;
    } else {
        // Nodo MIN: minimizamos y actualizamos beta
        double mejor = MasInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (val < mejor) mejor = val;
            beta = std::min(beta, mejor);
            if (beta <= alfa) break; // PODA ALFA: el MAX abuelo ya tiene algo mejor
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

// ============================================================
// Mejora D — evaluarVentana ajustada al modo 1-2-2-2
//
// En este juego cada jugador coloca HASTA 2 fichas por turno.
// Por tanto, con 3 fichas del rival en línea, el rival puede ganar
// colocando 2 fichas en su PRÓXIMO TURNO. Es una amenaza de muerte
// inminente, no de "dentro de 2 turnos" como en el ajedrez.
//
// Pesos ofensivos (fichas propias):
//   n-1 (4 de 5): ganamos EN ESTE turno                →  50000
//   n-2 (3 de 5): ganamos en el PRÓXIMO turno (2 fichas)→  10000
//   n-3 (2 de 5): necesitamos 2+ turnos más             →    100
//   1 ficha                                             →      1
//
// Pesos defensivos (fichas rivales) — siempre mayores:
//   n-1 (4 de 5): rival gana AHORA                     → -100000
//   n-2 (3 de 5): rival gana en su PRÓXIMO turno        →  -50000
//   n-3 (2 de 5): rival necesita 2 turnos               →    -500
//   1 ficha rival                                       →     -2
// ============================================================
static double evaluarVentana(int mias, int rival, int n) {
    // Ventana bloqueada: hay fichas de ambos jugadores → no puede ganar nadie
    if (mias > 0 && rival > 0) return 0.0;

    if (mias > 0) {
        if (mias == n - 1) return  50000.0; // 4 en raya: ganamos este turno
        if (mias == n - 2) return  10000.0; // 3 en raya: ganamos el siguiente (ponemos 2)
        if (mias == n - 3) return    100.0; // 2 en raya: necesitamos más turnos
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -100000.0; // 4 en raya rival: BLOQUEAR YA
        if (rival == n - 2) return  -50000.0; // 3 en raya rival: gana en su turno
        if (rival == n - 3) return    -500.0; // 2 en raya rival: amenaza a medio plazo
        return -2.0; // Peso ligeramente mayor al defensivo desde el principio
    }
    return 0.0;
}

// ============================================================
// HEURISTICA 1 — Función principal para la competición (-id1 1)
//
// Criterio 1: Victoria/Derrota inmediata (consistente con alfaBeta)
// Criterio 2: Alineaciones parciales en las 4 direcciones (evaluarVentana)
// Criterio 3: Control del centro del tablero
// Criterio 4: Casillas especiales (roja, verde, amarilla)
// Mejora E:   Multiplicador defensivo 1.5x cuando jugamos como J2
// ============================================================
double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Criterio 1: resultado inmediato (consistente con los valores de alfaBeta)
    if (ganador == id)       return  10000000.0;
    if (ganador == oponente) return -10000000.0;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar(); // 5 en modo competición
    double score = 0.0;

    // Criterio 2: alineaciones parciales en las 4 direcciones
    // Horizontal (df=0,dc=1), Vertical (1,0), Diagonal \ (1,1), Diagonal / (1,-1)
    const int dfs[] = { 0,  1,  1,  1};
    const int dcs[] = { 1,  0,  1, -1};

    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                // Extremo de la ventana de tamaño n
                int ef = f + df * (n - 1);
                int ec = c + dc * (n - 1);
                // Descartamos ventanas que se salgan del tablero
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;

                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f + df * k, c + dc * k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }

                // Mejora E — Modo Pánico J2:
                // Cuando somos J2, el ninja tiene ventaja de iniciativa (el 1-2-2-2
                // permite al J1 actuar primero). Para compensar, aplicamos un 
                // multiplicador 1.5x a todos los pesos defensivos cuando id == 2.
                // Así bloqueamos amenazas con más urgencia que J1.
                if (mias == 0 && rival > 0) {
                    double valorDefensivo = evaluarVentana(0, rival, n);
                    if (id == 2)
                        score += valorDefensivo * 1.5; // Defensa reforzada como J2
                    else
                        score += valorDefensivo;
                } else {
                    score += evaluarVentana(mias, rival, n);
                }
            }
        }
    }

    // Criterio 3: control del centro
    // Las casillas centrales del tablero 9x9 tienen más ventanas de 5 pasando
    // por ellas, lo que les da más potencial ofensivo y defensivo.
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
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            Tablero::TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == Tablero::TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);

            if (tipo == Tablero::TipoCelda::ROJO) {
                // Casilla roja: nuestra ficha se convirtió en del rival → muy malo
                if      (celda == id)       score -= 500.0;
                else if (celda == oponente) score += 500.0;

            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Casilla verde: da movimiento extra → bonus si la controlamos
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;

            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Casilla amarilla (bomba): calibrada entre -50000 y -100000.
                // Solo vale la pena activarla para romper amenazas de 4-en-raya
                // (-100000), nunca para 3-en-raya (-50000). Esto evita el
                // "doble-bomba" que dejaba al agente sin material.
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}

// ============================================================
// HEURISTICA 2 — Variante ultra-defensiva para comparación en la memoria.
// Misma estructura que heuristica1 pero con peso defensivo x2 en ventanas
// y menor bonus de centro (0.5 vs 1.0).
// Útil para la tabla comparativa de resultados de la memoria.
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
                // Peso defensivo x2: variante más conservadora
                if      (mias  > 0) score += evaluarVentana(mias,  0, n);
                else if (rival > 0) score += evaluarVentana(0, rival, n) * 2.0;
            }
        }
    }

    // Control del centro con menor peso que heuristica1
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

    // Casillas especiales (misma lógica que heuristica1)
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
