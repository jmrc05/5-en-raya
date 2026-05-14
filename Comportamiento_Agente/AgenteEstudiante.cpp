/**
 * @file AgenteEstudiante.cpp
 * @brief Implementación del agente inteligente para el juego 5-en-Raya Táctico (9x9).
 *
 * @details
 * Práctica 3 — Inteligencia Artificial — UGR — Curso 2025-2026
 *
 * Este fichero implementa un agente basado en búsqueda con adversario para
 * el juego 5-en-Raya Táctico. Las reglas especiales del juego condicionan
 * cada decisión de diseño de los algoritmos:
 *
 *  - **Turnos asimétricos 1-2-2-2**: J1 coloca 1 ficha en el primer turno;
 *    a partir de ahí ambos colocan hasta 2. Esto da ventaja ofensiva a J1.
 *  - **Regla de la Trinidad**: solo se puede colocar en (f,c) si
 *    `(f+c)%3 == faseActual%3`. Algunos huecos pueden ser inalcanzables
 *    en un turno concreto, creando amenazas matemáticamente imparables.
 *  - **Casillas especiales**: rojas (pierde la ficha), verdes (movimiento
 *    extra), amarillas (bomba que limpia fila y columna).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * ALGORITMOS IMPLEMENTADOS
 * ─────────────────────────────────────────────────────────────────────────
 *
 * **1. STATUS** — Resolución completa sin límite de profundidad.
 *    Determina si una posición es VICTORIA, DERROTA o EMPATE con juego
 *    óptimo de ambos. Solo viable en tableros pequeños (≤4×4) por su
 *    coste exponencial O(b^d).
 *
 * **2. MINIMAX** — Exploración del árbol de juego con límite de profundidad.
 *    Nodos MAX (nuestro turno) maximizan; nodos MIN (turno rival) minimizan.
 *    Profundidad máxima en competición: 4.
 *
 * **3. ALFA-BETA** — MiniMax con poda.
 *    Mantiene cotas alfa (lo mejor para MAX) y beta (lo mejor para MIN).
 *    - Poda BETA en MAX: si valor ≥ β, el MIN padre nunca elegiría aquí.
 *    - Poda ALFA en MIN: si valor ≤ α, el MAX abuelo ya tiene algo mejor.
 *    En el mejor caso reduce O(b^d) a O(b^(d/2)).
 *    Profundidad máxima en competición: 7 (con profundidad iterativa).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * MEJORAS IMPLEMENTADAS
 * ─────────────────────────────────────────────────────────────────────────
 *
 * **A. Profundidad Iterativa** (JuegaInteligente):
 *    Ejecuta alfaBeta de profundidad 1 hasta profundidadMax de forma
 *    incremental. Si el tiempo se agota, devuelve el mejor movimiento de
 *    la última iteración COMPLETA. Garantiza que nunca se devuelve un
 *    movimiento basado en una búsqueda incompleta.
 *
 * **B. Efecto Horizonte** (alfaBeta y minimax):
 *    Los nodos terminales devuelven `10000000 - profundidad` (victoria)
 *    o `-10000000 + profundidad` (derrota). El agente prefiere victorias
 *    rápidas y alarga las derrotas, en vez de tratarlas como igualmente
 *    malas independientemente de cuándo ocurran.
 *
 * **C. Move Ordering Táctico** (alfaBeta):
 *    Antes de explorar los hijos, los ordenamos por una puntuación táctica
 *    rápida que prioriza, en este orden:
 *      1. Bloqueo de 4-en-raya rival (+8000): máxima urgencia defensiva.
 *      2. Bloqueo de 3-en-raya rival (+4000): en 1-2-2-2 el rival gana
 *         en su próximo turno con solo 2 fichas más.
 *      3. Extensión de 4-en-raya propio (+3000): casi victoria.
 *      4. Extensión de 3-en-raya propio (+1000): amenaza seria.
 *      5. Casilla verde (+500): movimiento extra valioso pero no urgente.
 *      6. Centralidad como desempate: casillas centrales tienen más
 *         conexiones posibles.
 *    En la raíz aplica **Principal Variation Search**: el mejor movimiento
 *    de la iteración anterior (mejorMovimientoH) se coloca primero para
 *    maximizar los cortes desde el inicio.
 *
 * **D. Heurística Ajustada al 1-2-2-2** (evaluarVentana):
 *    Con 3 fichas rivales en línea, el rival puede ganar EN EL PRÓXIMO
 *    TURNO colocando 2 fichas → peso -50000. Los pesos defensivos son
 *    siempre mayores que los ofensivos equivalentes.
 *
 * **E. Modo Pánico J2** (heuristica1):
 *    Multiplicador 2.0× defensivo cuando id==2. J1 tiene ventaja de
 *    iniciativa estructural en el sistema 1-2-2-2: actúa primero y puede
 *    completar 5-en-raya antes de que J2 pueda bloquear. El multiplicador
 *    compensa esta asimetría haciendo al agente más paranoico ante amenazas
 *    rivales cuando juega como segundo jugador.
 *
 * **F. Casilla Amarilla Calibrada** (heuristica1):
 *    Penalización -75000: entre -50000 (3-en-raya) y -100000 (4-en-raya).
 *    El agente solo activará una bomba para romper amenazas de 4-en-raya,
 *    nunca de 3-en-raya. Evita el "doble-bomba" que dejaba al agente sin
 *    material y en posición estructuralmente perdedora.
 *
 * **G. Fix Casilla Roja** (heuristica1 y heuristica2):
 *    La lógica estaba invertida. La regla dice: quien coloca en rojo pierde
 *    esa ficha (se convierte en del rival). Por tanto:
 *      - `celda == oponente` en rojo → nosotros la pusimos y la perdimos → -500
 *      - `celda == id` en rojo → el rival la puso y nos la regaló       → +500
 *
 * **H. Casilla Verde Revalorizada** (heuristica1 y heuristica2):
 *    Aumentada de 80 a 5000. Una celda verde otorga +1 movimiento extra
 *    inmediato: en 1-2-2-2 permite colocar 3 fichas en un turno, lo que
 *    puede transformar una amenaza de 3-en-raya en victoria inmediata.
 *
 * **I. Conciencia de la Trinidad** (heuristica1):
 *    Si todos los huecos de una ventana con ≥3 fichas rivales son
 *    Trinity-inaccesibles en el turno actual, la amenaza es matemáticamente
 *    imparable este turno → multiplicador 3.0× sobre el peso defensivo.
 *    Fuerza al agente a detectar y prevenir estas situaciones con antelación.
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
 * @brief Algoritmo STATUS: resolución completa sin límite de profundidad.
 *
 * @details
 * Determina el resultado óptimo de una posición con juego perfecto de ambos.
 * Solo es viable en tableros pequeños (≤4×4) debido a su coste exponencial.
 *
 * Lógica de los nodos:
 *  - **MAX** (nuestro turno): VICTORIA > EMPATE > DERROTA.
 *  - **MIN** (turno rival): el rival elige DERROTA nuestra > EMPATE > VICTORIA nuestra.
 *
 * @param tablero Estado actual del juego.
 * @param Mov [Salida] La jugada óptima encontrada en la raíz.
 * @return VICTORIA, EMPATE o DERROTA con juego óptimo de ambos.
 */
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero& tablero, std::pair<int,int>& Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */

    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Casos base: estado terminal
    if (ganador == id)       return Resultado::VICTORIA;
    if (ganador == oponente) return Resultado::DERROTA;
    if (ganador == -1)       return Resultado::EMPATE;  // tablero lleno sin ganador

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

/**
 * @brief Algoritmo MiniMax clásico sin poda.
 *
 * @details
 * Explora el árbol de juego hasta profundidad prof_Max.
 * En nodos MAX (nuestro turno) elige el hijo con mayor valor heurístico.
 * En nodos MIN (turno rival) elige el hijo con menor valor heurístico.
 * Incorpora la Mejora B (efecto horizonte): los nodos terminales devuelven
 * `±10000000 ± profundidad` para preferir victorias rápidas y derrotas tardías.
 *
 * @param tablero Estado actual del juego.
 * @param profundidad Nivel actual en el árbol (0 = raíz).
 * @param prof_Max Límite de profundidad de búsqueda (competición: 4).
 * @param Mov [Salida] La mejor jugada encontrada (solo se actualiza en profundidad 0).
 * @return Valor heurístico del estado evaluado.
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

    // Mejora B — Efecto Horizonte: penalizamos la profundidad en nodos terminales.
    // Ganar en profundidad 2 vale más que ganar en profundidad 6.
    // Perder en profundidad 6 es mejor que perder en profundidad 2 (el rival puede equivocarse).
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
            double val = minimax(hijo, profundidad+1, prof_Max, movHijo);
            if (val > mejor) {
                mejor = val;
                // Guardamos la jugada solo en la raíz del árbol de búsqueda
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Seguridad: garantizamos que Mov siempre tiene un valor válido
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
        }
        return mejor;
    } else {
        // Nodo MIN: elegimos el hijo con menor valor heurístico
        double mejor = MasInfinito;
        for (const Tablero& hijo : sucesores) {
            std::pair<int,int> movHijo;
            double val = minimax(hijo, profundidad+1, prof_Max, movHijo);
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
 * @brief Punto de entrada para el juego inteligente con Profundidad Iterativa.
 *
 * @details
 * **Mejora A — Profundidad Iterativa:**
 * Ejecuta alfaBeta de profundidad 1 hasta profundidadMax de forma incremental.
 * Si el tiempo se agota durante la iteración d, descartamos ese resultado y
 * devolvemos el mejor movimiento de la iteración d-1 (completa). Esto garantiza
 * que nunca se devuelve un movimiento basado en una búsqueda incompleta.
 *
 * **Mejora C — Principal Variation Search (PVS):**
 * Tras cada iteración completa, guardamos el mejor movimiento en mejorMovimientoH.
 * alfaBeta lo coloca primero en la siguiente iteración, generando la mejor ventana
 * alfa-beta desde el inicio y maximizando los cortes.
 *
 * @param tablero Estado actual del juego.
 * @return La mejor jugada encontrada en la última iteración completa.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> mejorMov = {-1, -1};
    // Reseteamos el hint de PVS al inicio de cada turno
    mejorMovimientoH = {-1, -1};

    // Seguridad: inicializamos con el primer movimiento legal por si el tiempo
    // se agota antes de completar siquiera la profundidad 1
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty())
        mejorMov = SacarMovimiento(tablero, sucesores[0]);
    else
        return mejorMov;

    // Bucle de profundidad iterativa: de 1 hasta profundidadMax
    for (int prof = 1; prof <= profundidadMax; ++prof) {
        std::pair<int,int> movActual = {-1, -1};
        double valor = alfaBeta(tablero, 0, prof, MenosInfinito, MasInfinito, movActual);

        if (abortarBanda) {
            // Tiempo agotado: descartamos esta iteración y mantenemos la anterior
            std::cout << "--> Búsqueda cortada en profundidad " << prof << " por falta de tiempo.\n";
            break;
        }
        if (movActual.first != -1) {
            mejorMov         = movActual;
            mejorMovimientoH = movActual;  // Hint PVS para la siguiente iteración
            std::cout << "Profundidad " << prof << " completada. Valor Minimax: " << valor
                      << "\tJugada: (" << mejorMov.first << ", " << mejorMov.second << ")\n";
        }
    }
    return mejorMov;
}

/**
 * @brief Función auxiliar: puntúa tácticamente un movimiento para la ordenación.
 *
 * @details
 * Calcula una puntuación rápida O(4×n) para ordenar los sucesores en alfaBeta.
 * La ordenación correcta es crítica para la eficiencia de la poda alfa-beta:
 * si los mejores movimientos se exploran primero, la ventana se estrecha
 * rápidamente y se podan ramas enteras.
 *
 * Prioridades (de mayor a menor):
 *  1. Bloqueo de 4-en-raya rival (+8000): el rival gana en este turno si no bloqueamos.
 *  2. Bloqueo de 3-en-raya rival (+4000): en 1-2-2-2 el rival gana en 1 turno más.
 *  3. Extensión de 4-en-raya propio (+3000): podemos ganar este turno.
 *  4. Extensión de 3-en-raya propio (+1000): amenaza seria que el rival debe atender.
 *  5. Casilla verde (+500): movimiento extra valioso (no tan prioritario como bloquear).
 *  6. Centralidad como desempate final.
 *
 * Por qué no solo centralidad: ante ninja4, las amenazas críticas ocurren en
 * los bordes. Un ordering basado solo en el centro exploraría decenas de
 * movimientos inútiles antes de llegar al bloqueo urgente, agotando el tiempo.
 *
 * @param tablero Estado actual (antes del movimiento).
 * @param mov Movimiento candidato a puntuar.
 * @param id_jugador Identificador del jugador que realizará el movimiento.
 * @return Puntuación táctica del movimiento (mayor = más prometedor para MAX).
 */
static double puntuacionTactica(const Tablero& tablero, const std::pair<int,int>& mov, int id_jugador) {
    if (mov.first == -1) return 0.0;

    int oponente = (id_jugador == 1) ? 2 : 1;
    int n        = tablero.getNParaGanar();
    int filas    = tablero.getFilas();
    int cols     = tablero.getColumnas();
    double sc    = 0.0;

    // Prioridad 5: casilla verde (movimiento extra, valioso pero no urgente)
    if (tablero.getTipoCelda(mov.first, mov.second) == Tablero::TipoCelda::VERDE)
        sc += 500.0;

    // Prioridades 1-4: detección táctica en las 4 direcciones
    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        // Contamos fichas consecutivas desde el movimiento en ambas direcciones
        int piezasRival = 0, piezasMias = 0;

        for (int delta : {-1, 1}) {
            for (int k = 1; k < n; k++) {
                int ff = mov.first  + dfs[d] * delta * k;
                int cc = mov.second + dcs[d] * delta * k;
                if (ff < 0 || ff >= filas || cc < 0 || cc >= cols) break;
                int celda = tablero.getCelda(ff, cc);
                if      (celda == oponente) { piezasRival++; }
                else if (celda == id_jugador) { piezasMias++;  }
                else break;  // celda vacía interrumpe la cadena
            }
        }

        // Amenazas defensivas (bloquear al rival)
        if (piezasRival >= n - 1) sc += 8000.0;  // Bloqueamos 4-en-raya: máxima urgencia
        else if (piezasRival >= n - 2) sc += 4000.0;  // Bloqueamos 3-en-raya: rival gana en 1 turno

        // Amenazas ofensivas (extender nuestras líneas)
        if (piezasMias >= n - 1) sc += 3000.0;  // Completamos 4-en-raya: casi victoria
        else if (piezasMias >= n - 2) sc += 1000.0;  // Extendemos 3-en-raya
    }

    // Prioridad 6: centralidad como desempate (tablero 9x9, centro en (4,4))
    int dist = std::abs(mov.first - filas/2) + std::abs(mov.second - cols/2);
    sc -= dist;

    return sc;
}

/**
 * @brief Algoritmo Minimax con Poda Alfa-Beta y mejoras C y B.
 *
 * @details
 * **Poda Alfa-Beta:**
 *  - alfa = lo mejor que MAX puede garantizarse (inicia en -∞).
 *  - beta  = lo mejor que MIN puede garantizarse (inicia en +∞).
 *  - **Poda BETA** (en nodo MAX): si valor ≥ β, el nodo MIN padre nunca
 *    elegiría este camino → cortamos.
 *  - **Poda ALFA** (en nodo MIN): si valor ≤ α, el nodo MAX abuelo ya
 *    tiene algo mejor → cortamos.
 *
 * **Mejora B — Efecto Horizonte:**
 *  Victoria devuelve `10000000 - profundidad` y derrota `-10000000 + profundidad`.
 *  El agente prefiere ganar antes y perder lo más tarde posible.
 *
 * **Mejora C — Move Ordering Táctico + PVS:**
 *  Ordena los sucesores por `puntuacionTactica()` (O(4n) por nodo), que
 *  prioriza bloques defensivos sobre centralidad. En la raíz aplica
 *  Principal Variation Search colocando mejorMovimientoH primero.
 *
 * @param tablero Estado actual del juego.
 * @param profundidad Nivel actual en el árbol (0 = raíz).
 * @param prof_Max Límite de profundidad (competición: 7 con prof. iterativa).
 * @param alfa Cota inferior para MAX (valor mínimo garantizado).
 * @param beta  Cota superior para MIN (valor máximo garantizado).
 * @param Mov [Salida] La mejor jugada (solo se actualiza en profundidad 0).
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

    // Mejora C — Move Ordering Táctico:
    // Construimos (puntuación_táctica, índice) y ordenamos antes de explorar.
    // Para MAX: mayor puntuación primero (nuestros mejores movimientos).
    // Para MIN: menor puntuación primero (peores movimientos para nosotros).
    // Esto maximiza las podas desde el principio del árbol.
    std::vector<std::pair<double, int>> ranking;
    ranking.reserve(sucesores.size());
    for (int i = 0; i < (int)sucesores.size(); i++) {
        std::pair<int,int> mov = SacarMovimiento(tablero, sucesores[i]);
        ranking.push_back({puntuacionTactica(tablero, mov, id), i});
    }

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

    // Principal Variation Search (PVS):
    // En la raíz, colocamos el mejor movimiento de la iteración anterior primero.
    // Esto garantiza que alfa-beta empieza por la rama más prometedora según
    // la búsqueda de profundidad d-1, generando la mejor ventana posible y
    // maximizando los cortes en toda la iteración actual.
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
        // Poda BETA: si alfa ≥ beta, el nodo MIN padre nunca elegiría este camino.
        double mejor = MenosInfinito;
        for (auto& [sc, idx] : ranking) {
            const Tablero& hijo = sucesores[idx];
            std::pair<int,int> movHijo;
            double val = alfaBeta(hijo, profundidad+1, prof_Max, alfa, beta, movHijo);
            if (val > mejor) {
                mejor = val;
                // Guardamos la jugada solo en la raíz del árbol de búsqueda
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }
            // Seguridad: garantizamos que Mov siempre tiene un valor válido
            if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, hijo);
            alfa = std::max(alfa, mejor);
            if (alfa >= beta) break;  // PODA BETA
        }
        return mejor;
    } else {
        // Nodo MIN: minimizamos y actualizamos beta.
        // Poda ALFA: si beta ≤ alfa, el nodo MAX abuelo ya tiene algo mejor.
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
 * @brief Evalúa una ventana de n casillas consecutivas.
 *
 * @details
 * Calcula la puntuación de una ventana de tamaño n en el tablero.
 *
 * **Ajuste al sistema 1-2-2-2:**
 * Cada jugador coloca hasta 2 fichas por turno. Por tanto:
 *  - Con 3 fichas propias (n-2): ganamos EN EL PRÓXIMO TURNO colocando 2 fichas.
 *  - Con 3 fichas rivales (n-2): el rival gana EN SU PRÓXIMO TURNO.
 * Esto justifica que 3-en-raya sea casi tan urgente como 4-en-raya.
 *
 * **Asimetría defensiva:**
 * Los pesos defensivos son siempre mayores que los ofensivos equivalentes.
 * Si no bloqueamos una amenaza inmediata del rival, perdemos con certeza.
 *
 * Pesos ofensivos: 50000 / 10000 / 100 / 1
 * Pesos defensivos: 100000 / 50000 / 500 / 2
 *
 * @param mias  Número de fichas propias en la ventana.
 * @param rival Número de fichas rivales en la ventana.
 * @param n     Tamaño de la ventana (número de fichas para ganar).
 * @return Puntuación de la ventana (positiva = ventaja propia, negativa = ventaja rival).
 */
static double evaluarVentana(int mias, int rival, int n) {
    // Ventana bloqueada: hay fichas de ambos jugadores → nadie puede ganar aquí
    if (mias > 0 && rival > 0) return 0.0;

    if (mias > 0) {
        if (mias == n - 1) return  50000.0;  // 4-en-raya: ganamos este turno
        if (mias == n - 2) return  10000.0;  // 3-en-raya: ganamos el siguiente (ponemos 2)
        if (mias == n - 3) return    100.0;  // 2-en-raya: amenaza a medio plazo
        return 1.0;
    }
    if (rival > 0) {
        if (rival == n - 1) return -100000.0;  // 4-en-raya rival: BLOQUEAR AHORA
        if (rival == n - 2) return  -50000.0;  // 3-en-raya rival: gana en su turno
        if (rival == n - 3) return    -500.0;  // 2-en-raya rival: amenaza a medio plazo
        return -2.0;
    }
    return 0.0;
}

/**
 * @brief Heurística principal para la competición (activada con -id1 1).
 *
 * @details
 * Evalúa la calidad de una posición para el jugador `id`. Combina cinco criterios:
 *
 * **Criterio 1 — Victoria/Derrota inmediata:**
 * Devuelve ±10000000 (consistente con los valores de alfaBeta) para garantizar
 * que cualquier victoria/derrota inmediata domina sobre cualquier evaluación
 * heurística.
 *
 * **Criterio 2 — Alineaciones parciales (evaluarVentana):**
 * Evalúa todas las ventanas de tamaño n en las 4 direcciones (horizontal,
 * vertical, diagonal ↘, diagonal ↗). Los pesos están ajustados al sistema
 * 1-2-2-2 (ver evaluarVentana).
 *
 * **Mejora E — Modo Pánico J2:**
 * Cuando id==2, aplica un multiplicador 2.0× a todos los pesos defensivos.
 * El Jugador 1 tiene ventaja estructural de iniciativa en 1-2-2-2: actúa
 * primero y puede completar líneas antes de que J2 pueda bloquear.
 *
 * **Mejora I — Conciencia de la Trinidad:**
 * Si todos los huecos de una ventana con ≥3 fichas rivales tienen
 * `(f+c)%3 ≠ faseActual%3` (Trinity-inaccesibles), la amenaza es
 * matemáticamente imparable en el turno actual → multiplicador 3.0×.
 * Esto fuerza al agente a detectar y prevenir estas situaciones con
 * antelación en vez de descubrirlas cuando ya es demasiado tarde.
 *
 * **Criterio 3 — Control del centro:**
 * Las casillas centrales del tablero 9×9 tienen más ventanas de n pasando
 * por ellas → más potencial ofensivo y defensivo. Bonus proporcional a
 * la proximidad al centro (4,4).
 *
 * **Criterio 4 — Casillas especiales:**
 *  - **Roja (Mejora G — Fix):** La lógica estaba invertida. Quien coloca
 *    en rojo pierde la ficha (se convierte en del rival). Por tanto:
 *    `celda == oponente` → nosotros la pusimos, la perdimos → -500.
 *    `celda == id` → el rival la puso, nos la regaló → +500.
 *  - **Verde (Mejora H — Revalorizada):** Aumentada de 80 a 5000.
 *    Una celda verde da +1 movimiento extra: en 1-2-2-2 permite 3 fichas
 *    en un turno, transformando una amenaza de 3-en-raya en victoria.
 *  - **Amarilla (Mejora F — Calibrada):** Penalización -75000, entre
 *    -50000 (3-en-raya) y -100000 (4-en-raya). Solo activar bombas para
 *    romper amenazas de 4-en-raya, nunca de 3-en-raya.
 *
 * @param tablero Estado del juego a evaluar.
 * @return Puntuación numérica (positiva = ventaja propia, negativa = ventaja rival).
 */
double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int oponente = (id == 1) ? 2 : 1;
    int ganador  = tablero.comprobarGanador();

    // Criterio 1: resultado inmediato (consistente con los valores de alfaBeta)
    if (ganador == id)       return  10000000.0;
    if (ganador == oponente) return -10000000.0;
    if (ganador == -1)       return  0.0;

    int filas = tablero.getFilas();
    int cols  = tablero.getColumnas();
    int n     = tablero.getNParaGanar();  // 5 en modo competición
    double score = 0.0;

    // Fase actual para la Mejora I (conciencia de la Trinidad)
    int fase = tablero.getFaseActual();

    // Criterio 2: alineaciones parciales en las 4 direcciones
    // Horizontal (0,1) | Vertical (1,0) | Diagonal ↘ (1,1) | Diagonal ↗ (1,-1)
    const int dfs[] = {0, 1, 1, 1};
    const int dcs[] = {1, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        int df = dfs[d], dc = dcs[d];
        for (int f = 0; f < filas; f++) {
            for (int c = 0; c < cols; c++) {
                // Verificamos que la ventana cabe en el tablero
                int ef = f + df*(n-1), ec = c + dc*(n-1);
                if (ef < 0 || ef >= filas || ec < 0 || ec >= cols) continue;

                // Contamos fichas propias y rivales en la ventana
                int mias = 0, rival = 0;
                for (int k = 0; k < n; k++) {
                    int celda = tablero.getCelda(f+df*k, c+dc*k);
                    if      (celda == id)       mias++;
                    else if (celda == oponente) rival++;
                }

                if (mias == 0 && rival > 0) {
                    // Ventana con solo fichas rivales: aplicamos pesos defensivos

                    // Mejora I — Conciencia de la Trinidad:
                    // Comprobamos si algún hueco vacío de la ventana es accesible
                    // este turno según la Regla de la Trinidad.
                    // Si NINGÚN hueco es accesible, la amenaza es imparable → x3.
                    double factorTrinidad = 1.0;
                    if (rival >= n - 2) {  // Solo comprobamos amenazas serias (≥3 fichas)
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
                        // Si todos los huecos son Trinity-bloqueados: amenaza imparable
                        if (!hayHuecoAccesible) factorTrinidad = 3.0;
                    }

                    double val = evaluarVentana(0, rival, n) * factorTrinidad;

                    // Mejora E — Modo Pánico J2: multiplicador 2.0× defensivo cuando id==2.
                    // Compensa la ventaja de iniciativa estructural de J1 en 1-2-2-2.
                    score += (id == 2) ? val * 2.0 : val;

                } else {
                    // Ventana con fichas propias (o ambos): evaluación estándar
                    score += evaluarVentana(mias, rival, n);
                }
            }
        }
    }

    // Criterio 3: control del centro
    // Casillas centrales tienen más ventanas de n → más conexiones posibles
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
                // Mejora G — Fix casilla roja (lógica estaba INVERTIDA):
                // Quien coloca en rojo pierde esa ficha (pasa al rival).
                // celda == oponente → nosotros la pusimos, la perdimos → penalizar
                // celda == id       → el rival la puso, nos la regaló  → bonificar
                if      (celda == oponente) score -= 500.0;
                else if (celda == id)       score += 500.0;

            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Mejora H — Verde revalorizada de 80 a 5000:
                // +1 movimiento extra permite colocar 3 fichas en 1 turno (1-2-2-2).
                // Una amenaza de 3-en-raya se convierte en victoria inmediata.
                if      (celda == id)       score += 5000.0;
                else if (celda == oponente) score -= 5000.0;

            } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                // Mejora F — Calibrada entre -50000 (3-en-raya) y -100000 (4-en-raya):
                // Solo vale la pena activar una bomba para romper amenazas de 4-en-raya.
                // Si el umbral es -75000, activar bomba cuesta más que romper 3-en-raya
                // (-50000) pero menos que evitar 4-en-raya (-100000).
                if      (celda == id)       score -= 75000.0;
                else if (celda == oponente) score += 10000.0;
            }
        }
    }

    return score;
}

/**
 * @brief Heurística alternativa ultra-defensiva (activada con -id1 2).
 *
 * @details
 * Variante de heuristica1 diseñada para comparación experimental en la memoria.
 * Diferencias respecto a heuristica1:
 *  - Peso defensivo ×2.0 en ventanas (vs ×1.0 en heuristica1).
 *  - Bonus de centro 0.5 (vs 1.0 en heuristica1).
 *  - Sin conciencia de la Trinidad (para comparar su impacto).
 *
 * @param tablero Estado del juego a evaluar.
 * @return Puntuación numérica (positiva = ventaja propia, negativa = ventaja rival).
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

    // Alineaciones parciales con peso defensivo x2.0 (sin conciencia de Trinidad)
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

    // Control del centro con menor peso (0.5 vs 1.0 en heuristica1)
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

    // Casillas especiales (misma lógica corregida que heuristica1)
    for (int f = 0; f < filas; f++) {
        for (int c = 0; c < cols; c++) {
            Tablero::TipoCelda tipo = tablero.getTipoCelda(f, c);
            if (tipo == Tablero::TipoCelda::NORMAL) continue;
            int celda = tablero.getCelda(f, c);
            if (tipo == Tablero::TipoCelda::ROJO) {
                // Fix G: lógica corregida (igual que heuristica1)
                if      (celda == oponente) score -= 500.0;
                else if (celda == id)       score += 500.0;
            } else if (tipo == Tablero::TipoCelda::VERDE) {
                // Mejora H: verde revalorizada (igual que heuristica1)
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
