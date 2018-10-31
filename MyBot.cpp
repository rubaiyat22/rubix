#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"

#include <random>
#include <ctime>
#include <queue>

using namespace std;
using namespace hlt;


struct CompareCells {
    bool operator()(MapCell* left, MapCell* right) {
        return (left->halite - left->distance_from_shipyard) < (right->halite - right->distance_from_shipyard);
    }
};

int main(int argc, char* argv[]) {
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    } else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    shared_ptr<Player> me = game.me;
    unique_ptr<GameMap>& game_map = game.game_map;

    Position shipyard_pos = me->shipyard->position;
    
    int ALL_CELLS = game.game_map->width * game.game_map->height;
    vector<MapCell*> cells_around_shipyard;
    for (int x = -2; x < 2; x++) {
        for (int y = -2; y < 2; y++) {
            MapCell* cell = game_map->at(Position(shipyard_pos.x + x, shipyard_pos.y + y));
            cell->distance_from_shipyard = abs(x) + abs(y);
            cells_around_shipyard.push_back(cell);
        }
    }

    priority_queue<MapCell*, vector<MapCell*>, CompareCells> cells(cells_around_shipyard.begin(), cells_around_shipyard.end());

    game.ready("MyCppBot");

    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");

    for (;;) {
        game.update_frame();
        shared_ptr<Player> me = game.me;
        unique_ptr<GameMap>& game_map = game.game_map;

        vector<Command> command_queue;

        for (const auto& ship_iterator : me->ships) {
            shared_ptr<Ship> ship = ship_iterator.second;
            if (game_map->at(ship)->halite < constants::MAX_HALITE / 10 || ship->is_full()) {
                Direction random_direction = ALL_CARDINALS[rng() % 4];
                command_queue.push_back(ship->move(random_direction));
            } else {
                command_queue.push_back(ship->stay_still());
            }
        }

        int all_ships = 0;
        for (const auto& player: game.players) {
            all_ships += player->ships.size();
        }
        float load_factor = all_ships/ALL_CELLS;
        if (load_factor < 0.75 &&
            me->halite >= constants::SHIP_COST &&
            !game_map->at(me->shipyard)->is_occupied()) {
            command_queue.push_back(me->shipyard->spawn());
        }

        if (!game.end_turn(command_queue)) {
            break;
        }
    }

    return 0;
}
