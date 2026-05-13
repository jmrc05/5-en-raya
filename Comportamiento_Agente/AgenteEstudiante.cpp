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
std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero &hijo){
    for(int f=0; f<padre.getFilas(); ++f)
        for(int c=0; c<padre.getColumnas(); ++c)
            if (padre.getCelda(f,c) == 0 && hijo.getCelda(f,c) != 0) 
                return {f, c};
    return {-1, -1};
}

/**
 * @brief Implementa un agente que juega de forma totalmente aleatoria.
 */
std::pair<int, int> AgenteEstudiante::JuegaAleatorio(const Tablero& tablero) {
    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return {-1, -1};
    int elegido = rand() % sucesores.size();
    std::pair<int,int> Mov = SacarMovimiento(tablero, sucesores[elegido]);
    return Mov;
}

/**
 * @brief Algoritmo Status — resolución completa sin límite de profundidad.
 * Solo viable en tableros pequeños (3x3, 4x4).
 */
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero &tablero, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    if (ganador == id)       return Resultado::VICTORIA;
    if (ganador == oponente) return Resultado::DERROTA;
    if (ganador == -1)       return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty())   return Resultado::EMPATE;

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
 * @brief Implementación del algoritmo Minimax clásico (profundidad competición: 4).
 */
double AgenteEstudiante::minimax(const Tablero &tablero, int profundidad, int prof_Max, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int ganador = tablero.comprobarGanador();
    if (ganador != 0 || profundidad == prof_Max) return heuristica(tablero);

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
 * @brief Punto de entrada para el juego inteligente con profundidad iterativa.
 * La profundidad iterativa garantiza que siempre tenemos un movimiento válido
 * aunque se agote el tiempo, usando el mejor resultado de la última profundidad completa.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};

    // Inicializamos con el primer movimiento legal como seguridad ante timeout
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) {
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    } else {
        return mejorMov;
    }

    // Profundidad iterativa: de 1 hasta profundidadMax
    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);

        if (abortarBanda) {
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break; 
        }
        if (movActual.first != -1) {
            mejorMov = movActual;
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor 
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

/**
 * @brief Implementación del algoritmo Minimax con Poda Alfa-Beta (profundidad competición: 7).
 *
 * Mejoras sobre Minimax puro:
 * 1. Poda alfa-beta: elimina ramas que no pueden mejorar el resultado conocido.
 *    - alfa = lo mejor que MAX puede garantizarse (empieza en -inf)
 *    - beta = lo mejor que MIN puede garantizarse (empieza en +inf)
 *    - Poda BETA (en MAX): si valor >= beta, el MIN padre no elegiría aquí → cortamos
 *    - Poda ALFA (en MIN): si valor <= alfa, el MAX abuelo ya tiene algo mejor → cortamos
 * 2. Ordenación de sucesores: exploramos primero los movimientos más prometedores
 *    usando heuristicaPrueba (rápida) para ordenar. Esto maximiza las podas.
 */
double AgenteEstudiante::alfaBeta(const Tablero &tablero, int profundidad, int prof_Max, double alfa, double beta, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int ganador = tablero.comprobarGanador();
    if (ganador != 0 || profundidad == prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    // ORDENACIÓN DE SUCESORES: evaluamos cada hijo con heuristicaPrueba (O(n) rápida)
    // y los ordenamos de mejor a peor antes de explorar. Esto hace que alfa-beta pode
    // muchas más ramas, permitiendo llegar a mayor profundidad en el mismo tiempo.
    bool esMax = (tablero.getJugadorTurno() == id);
    std::vector<std::pair<double, int>> puntuaciones; // (score, índice)
    puntuaciones.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++) {
        puntuaciones.push_back({heuristicaPrueba(sucesores[i]), i});
    }
    // MAX ordena de mayor a menor (mejor para nosotros primero)
    // MIN ordena de menor a mayor (peor para nosotros primero)
    if (esMax)
        std::sort(puntuaciones.begin(), puntuaciones.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
    else
        std::sort(puntuaciones.begin(), puntuaciones.end(),
                  [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first < b.first; });

    if (esMax) {
        // Nodo MAX: maximizamos y actualizamos alfa
        double mejor = MenosInfinito;
        for (auto& [score, idx] : puntuaciones) {
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
        // Nodo MIN: minimizamos y actualizamos beta
        double mejor = MasInfinito;
        for (auto& [score, idx] : puntuaciones) {
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
        case 1: return heuristica1(tablero); break;
        case 2: return heuristica2(tablero); break;
        default: return heuristica1(tablero);
    }
}

/**
 * @brief heuristicaPrueba — NO MODIFICAR.
 * Evalúa centralidad de fichas. Usada también para la ordenación de sucesores
 * en alfaBeta por ser O(n) y muy rápida.
 */
double AgenteEstudiante::heuristicaPrueba(const Tablero& tablero) {
    int n = tablero.getNParaGanar();
    int oponente = (id == 1) ? 2 : 1;
    double score_positivo = 0;
    double score_negativo = 0;
    for (int f=0; f< tablero.getFilas(); f++){
        for (int c = 0; c< tablero.getColumnas(); c++){
            if (tablero.getCelda(f,c) != 0){
                int valor = tablero.getFilas()-abs(f-(tablero.getFilas()/2)) + tablero.getColumnas()-abs(c-(tablero.getColumnas()/2)); 
                if (tablero.getCelda(f,c) == id) score_positivo += valor;
                else score_negativo += valor;
            }
        }
    }
    return score_positivo - score_negativo;
}

// ============================================================
// Función auxiliar: evalúa una ventana de n celdas consecutivas.
//
// Pesos ofensivos (fichas propias):
//   n-1 fichas → 100000   (un paso de ganar)
//   n-2 fichas → 1000
//   n-3 fichas → 10
//   1 ficha    → 1
//
// Pesos defensivos (fichas rivales) — deliberadamente el DOBLE:
// La teoría de juegos nos dice que bloquear una amenaza inmediata
// del rival vale más que crear una propia equivalente, porque si
// no la bloqueamos el rival gana con certeza.
//   n-1 fichas → 200000
//   n-2 fichas → 3000
//   n-3 fichas → 30
//   1 ficha    → 1
// ============================================================
static double evaluarVentana(int mias, int rival, int n) {
    if (mias > 0 && rival > 0) return 0.0; // ventana bloqueada

    if (mias > 0) {
        if (mias == n - 1) return  100000.0;
        if (mias == n - 2) return    1000.0;
        if (mias == n - 3) return      10.0;
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -200000.0;
        if (rival == n - 2) return   -3000.0;
        if (rival == n - 3) return     -30.0;
        return -1.0;
    }
    return 0.0;
}

/**
 * @brief heuristica1 — Heurística principal para la competición (-id1 1).
 *
 * Criterios (de mayor a menor importancia):
 *
 * 1. Victoria/Derrota inmediata → ±GANAR (1e12)
 *
 * 2. Alineaciones parciales en las 4 direcciones:
 *    Evaluamos cada ventana de tamaño n con evaluarVentana().
 *    Las ventanas con fichas de ambos jugadores valen 0 (bloqueadas).
 *    Los pesos defensivos son el doble que los ofensivos.
 *
 * 3. Control del centro:
 *    Las casillas centrales del tablero 9x9 tienen más conexiones posibles
 *    (más ventanas de 5 pasan por ellas). Bonus proporcional a la proximidad.
 *
 * 4. Casillas especiales:
 *    - Roja: nuestra ficha fue convertida al rival → penalización -500
 *    - Verde: da movimiento extra → bonus ±80 según quién la controla
 *    - Amarilla (bomba): si nuestra ficha la activó → penalización -5000
 *      (destruimos nuestro propio setup). Si el rival la activó → +1000.
 */
double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Criterio 1: victoria / derrota / empate técnico
    if (ganador == id)       return  GANAR;
    if (ganador == oponente) return  PERDER;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar(); // 5 en modo competición

    double score = 0.0;

    // Criterio 2: alineaciones parciales en las 4 direcciones
    // Horizontal (0,1), Vertical (1,0), Diagonal \ (1,1), Diagonal / (1,-1)
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
    int centro_f = filas / 2;
    int centro_c = cols  / 2;
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
                // Nuestra ficha se convirtió en del rival → muy malo
                if      (celda == id)       score -= 500.0;
                else if (celda == oponente) score += 500.0;
            }
            else if (tipo == Tablero::TipoCelda::VERDE) {
                // Da movimiento extra → bueno si la controlamos
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;
            }
            else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Bomba: si nuestra ficha está aquí, significa que la activamos
                // y destruimos todo en su fila y columna (incluido nuestro setup)
                if      (celda == id)       score -= 5000.0;
                else if (celda == oponente) score += 1000.0;
            }
        }
    }

    return score;
}

/**
 * @brief heuristica2 — Variante más defensiva para comparación.
 * Idéntica a heuristica1 pero con peso defensivo x2 en ventanas
 * y menor bonus de centro (0.3 vs 0.5).
 * Útil para la tabla comparativa de la memoria.
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
                else if (rival > 0) score += evaluarVentana(0, rival, n) * 2.0; // más defensivo
            }
        }
    }

    // Control del centro con menor peso
    int centro_f = filas / 2;
    int centro_c = cols  / 2;
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

    // Casillas especiales
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
                if      (celda == id)       score -= 5000.0;
                else if (celda == oponente) score += 1000.0;
            }
        }
    }

    return score;
}
