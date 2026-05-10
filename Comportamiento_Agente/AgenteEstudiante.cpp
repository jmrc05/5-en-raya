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
 * @param padre Estado inicial del tablero.
 * @param hijo Estado resultante tras un movimiento.
 * @return Un par (fila, columna) con la posición de la nueva pieza.
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
 * @param tablero Estado actual del juego.
 * @return La jugada elegida al azar.
 */
std::pair<int, int> AgenteEstudiante::JuegaAleatorio(const Tablero& tablero) {

    // Calculo los tableros descendientes de tablero
    auto sucesores = tablero.getSucesores();

    // Si no tiene descendientes, paso el turno
    if (sucesores.empty()) return {-1, -1};

    // Elijo aleatoriamente uno de los descendientes
    int elegido = rand() % sucesores.size();

    // Saco el movimiento realizado comparando el tablero original con el elegido.
    std::pair<int,int> Mov = SacarMovimiento(tablero, sucesores[elegido]);

    return Mov;
}


/**
 * @brief Algoritmo de resolución completa para estados de final de juego.
 * Determina si una posición está matemáticamente ganada, perdida o empatada.
 * @param tablero Estado a evaluar.
 * @param Mov [Salida] La jugada óptima encontrada.
 * @return Resultado del análisis (VICTORIA, DERROTA o EMPATE).
 */
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero &tablero, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Caso base: ya hay un ganador o empate técnico (tablero lleno)
    if (ganador == id)       return Resultado::VICTORIA;
    if (ganador == oponente) return Resultado::DERROTA;
    if (ganador == -1)       return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty())   return Resultado::EMPATE;

    bool hayEmpate = false;
    std::pair<int,int> movEmpate = {-1, -1};

    if (tablero.getJugadorTurno() == id) {
        // Nodo MAX: buscamos la mejor opción para nosotros
        // Prioridad: VICTORIA > EMPATE > DERROTA
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            Resultado r = Status(hijo, movHijo);
            std::pair<int,int> mov = SacarMovimiento(tablero, hijo);
            if (r == Resultado::VICTORIA) {
                Mov = mov;
                return Resultado::VICTORIA;
            }
            if (r == Resultado::EMPATE && !hayEmpate) {
                hayEmpate = true;
                movEmpate = mov;
            }
        }
        if (hayEmpate) { Mov = movEmpate; return Resultado::EMPATE; }
        Mov = SacarMovimiento(tablero, sucesores[0]);
        return Resultado::DERROTA;

    } else {
        // Nodo MIN: el rival elige la peor opción para nosotros
        // Prioridad del rival: DERROTA nuestra > EMPATE > VICTORIA nuestra
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
 * @brief Implementación del algoritmo Minimax clásico.
 * @param tablero Estado actual.
 * @param profundidad Nivel actual en el árbol de búsqueda.
 * @param prof_Max Límite de profundidad de la búsqueda.
 * @param Mov [Salida] La mejor jugada encontrada en la raíz.
 * @return Valor heurístico del estado.
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

    // Caso base: victoria/derrota, profundidad máxima o sin movimientos
    if (ganador != 0 || profundidad == prof_Max)
        return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty())
        return heuristica(tablero);

    if (tablero.getJugadorTurno() == id) {
        // Nodo MAX: elegimos el hijo con mayor valor
        double mejor = MenosInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val > mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Garantizamos que al menos el primer movimiento queda guardado
            if (profundidad == 0 && Mov.first == -1)
                Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;

    } else {
        // Nodo MIN: elegimos el hijo con menor valor
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad + 1, prof_Max, movHijo);
            if (val < mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            if (profundidad == 0 && Mov.first == -1)
                Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    }
}


/**
 * @brief Punto de entrada para el juego inteligente.
 * @param tablero Estado actual del juego.
 * @return La jugada elegida por el algoritmo de búsqueda.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};

    // 1. PARCHE DE SEGURIDAD: Inicializamos con el primer movimiento legal posible.
    // Así, si el tiempo se agota instantáneamente (por lag), jamás devolveremos {-1, -1}.
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) {
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    } else {
        return mejorMov; // El juego ya ha terminado
    }

    // 2. PROFUNDIDAD ITERATIVA
    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};
        
        // Buscamos a la profundidad actual
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);
        
        // Si la búsqueda fue interrumpida por falta de tiempo, la descartamos
        // y nos quedamos con el mejorMov de la iteración anterior (que sí terminó).
        if (abortarBanda) {
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break; 
        }
        
        // Si dio tiempo a completar esta profundidad, actualizamos nuestro mejor movimiento seguro
        if (movActual.first != -1) {
            mejorMov = movActual;
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor 
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }

    return mejorMov;
}


/**
 * @brief Implementación del algoritmo Minimax con Poda Alfa-Beta.
 * @param tablero Estado actual.
 * @param profundidad Nivel actual en el árbol de búsqueda.
 * @param prof_Max Límite de profundidad de la búsqueda.
 * @param alfa Valor mínimo garantizado para el jugador MAX.
 * @param beta Valor máximo garantizado para el jugador MIN.
 * @param Mov [Salida] La mejor jugada encontrada en la raíz.
 * @return Valor heurístico del estado tras la poda.
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

    // Caso base: victoria/derrota, profundidad máxima o sin movimientos
    if (ganador != 0 || profundidad == prof_Max)
        return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty())
        return heuristica(tablero);

    if (tablero.getJugadorTurno() == id) {
        // Nodo MAX: maximizamos y actualizamos alfa
        // Poda BETA: si alfa >= beta, el nodo MIN padre nunca elegiría este camino
        double mejor = MenosInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (val > mejor) {
                mejor = val;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Garantizamos que al menos el primer movimiento queda guardado
            if (profundidad == 0 && Mov.first == -1)
                Mov = SacarMovimiento(tablero, hijo);
            alfa = std::max(alfa, mejor);
            if (alfa >= beta) break; // PODA BETA
        }
        return mejor;

    } else {
        // Nodo MIN: minimizamos y actualizamos beta
        // Poda ALFA: si beta <= alfa, el nodo MAX abuelo ya tiene algo mejor
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (val < mejor) {
                mejor = val;
            }
            beta = std::min(beta, mejor);
            if (beta <= alfa) break; // PODA ALFA
        }
        return mejor;
    }
}

/**
 * @brief Función heurística para evaluar la calidad de un tablero.
 * @param tablero Estado a evaluar.
 * @return Puntuación numérica (positiva para ventaja de J1, negativa para J2).
 */
double AgenteEstudiante::heuristica(const Tablero& tablero) {
    switch(numHeuristica) {
        case 0: return heuristicaPrueba(tablero);
                break;
        case 1: return heuristica1(tablero);
                break;
        case 2: return heuristica2(tablero);
                break;
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
// Función auxiliar: evalúa una ventana de n celdas consecutivas
//
// Si hay fichas de ambos jugadores → ventana bloqueada → 0
// Si solo hay fichas propias:
//   n-1 fichas (4 de 5) → 100000  (amenaza inmediata de victoria)
//   n-2 fichas (3 de 5) → 1000
//   n-3 fichas (2 de 5) → 10
//   1 ficha              → 1
// Si solo hay fichas rivales: mismos valores pero negativos
// ============================================================
static double evaluarVentana(int mias, int rival, int n) {
    if (mias > 0 && rival > 0) return 0.0;

    if (mias > 0) {
        if (mias == n - 1) return  100000.0;
        if (mias == n - 2) return    1000.0;
        if (mias == n - 3) return      10.0;
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -100000.0;
        if (rival == n - 2) return   -1000.0;
        if (rival == n - 3) return     -10.0;
        return -1.0;
    }
    return 0.0;
}

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
                // Descartamos ventanas que se salgan del tablero
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
    // Las casillas centrales tienen más conexiones posibles
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
    // Roja: penalizamos ficha propia (fue convertida al rival)
    // Verde: bonificamos si la controlamos (da movimiento extra)
    // Amarilla: penalizamos fichas propias expuestas en su fila/columna
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);

            if (tipo == TipoCelda::ROJO) {
                if      (celda == id)       score -= 300.0;
                else if (celda == oponente) score += 300.0;
            }
            else if (tipo == TipoCelda::VERDE) {
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;
            }
            else if (tipo == TipoCelda::AMARILLO) {
                for (int ff = 0; ff < filas; ff++) {
                    int v = tablero.getCelda(ff, c);
                    if      (v == id)       score -= 20.0;
                    else if (v == oponente) score += 20.0;
                }
                for (int cc = 0; cc < cols; cc++) {
                    int v = tablero.getCelda(f, cc);
                    if      (v == id)       score -= 20.0;
                    else if (v == oponente) score += 20.0;
                }
            }
        }
    }

    return score;
}

double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Criterio 1: victoria / derrota / empate técnico
    if (ganador == id)       return  GANAR;
    if (ganador == oponente) return  PERDER;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();

    double score = 0.0;

    // Criterio 2: igual que heuristica1 pero con mayor peso defensivo
    // Las amenazas del rival valen 1.5x más → el agente bloquea más
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
                else if (rival > 0) score += evaluarVentana(0, rival, n) * 1.5;
            }
        }
    }

    // Criterio 3: control del centro con peso reducido (0.3 vs 0.5)
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

    // Criterio 4: casillas especiales
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);
            if (tipo == TipoCelda::ROJO) {
                if      (celda == id)       score -= 300.0;
                else if (celda == oponente) score += 300.0;
            } else if (tipo == TipoCelda::VERDE) {
                if      (celda == id)       score +=  80.0;
                else if (celda == oponente) score -=  80.0;
            }
        }
    }

    return score;
}
