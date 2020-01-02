#include <vector>
#include <algorithm>
#include <fmt/core.h>
#include <array>
#include <iostream>
#include <range/v3/all.hpp>
#include <random>

template<typename ...T>
constexpr int make_board(T ... positions) {
	return ((0b1 << positions) | ...);
}

static_assert(make_board(1, 2, 3) == 0b1110);
static_assert(make_board(0, 1, 2, 3) == 0b1111);
static_assert(make_board(0, 1, 2) == 0b111);

bool is_winning_board(int board) {
	static constexpr std::array<int, 8> winning_boards = {
		0b111,
		0b111 << 3,
		0b111 << 6,
		make_board(0, 3, 6),
		make_board(1, 4, 7),
		make_board(2, 5, 8),
		make_board(0, 4, 8),
		make_board(2, 4, 6)
	};


	return std::any_of(winning_boards.begin(), winning_boards.end(), [&](const int b2) {
		return (b2 & board) == b2;
	});
}

//012
//345
//678

constexpr int full_board = 0b111111111;

char letter_at_idx(int idx, int o_board, int x_board) {
	if (o_board & 0b1 << idx) {
		return 'O';
	} else if (x_board & 0b1 << idx) {
		return 'X';
	} else {
		return ' ';
	}

}

void print_board(int o_board, int x_board) {
	fmt::print("|{}|{}|{}|\n",
			   letter_at_idx(0, o_board, x_board),
			   letter_at_idx(1, o_board, x_board),
			   letter_at_idx(2, o_board, x_board)
	);
	fmt::print("------\n");
	fmt::print("|{}|{}|{}|\n",
			   letter_at_idx(3, o_board, x_board),
			   letter_at_idx(4, o_board, x_board),
			   letter_at_idx(5, o_board, x_board)
	);
	fmt::print("------\n");
	fmt::print("|{}|{}|{}|\n",
			   letter_at_idx(6, o_board, x_board),
			   letter_at_idx(7, o_board, x_board),
			   letter_at_idx(8, o_board, x_board)
	);
	fmt::print("------\n");
}

struct tictactoe_board {
	int o_board = 0;
	int x_board = 0;

	int combined_board() const noexcept {
		return x_board | o_board;
	}

	bool is_full_board() const {
		return combined_board() == full_board;
	}

};

template<typename AI1, typename AI2>
struct tic_tac_toe_game {

	tic_tac_toe_game(AI1 a, AI2 b):
		m_o_player(std::move(a)),
		m_x_player(std::move(b)) {}

	static constexpr int o_wins = 1;
	static constexpr int x_wins = 2;
	static constexpr int tie = 0;

	int run() {
		while (m_board.combined_board() != full_board) {
			print_board(m_board.o_board, m_board.x_board);
			fmt::print("\n");
			const bool is_o_turn = move_count % 2 == 0;
			int move_number = 0;
			do {
				if (is_o_turn) {
					move_number = m_o_player.next_move(m_board);
				} else {
					move_number = m_x_player.next_move(m_board);
				}
			} while (m_board.combined_board() & (0b1 << move_number));


			int& current_board = [&]()->auto& {
				if (is_o_turn) {
					return m_board.o_board;
				} else {
					return m_board.x_board;
				}
			}();

			current_board |= 0b1 << move_number;

			if (is_winning_board(current_board)) {
				print_board(m_board.o_board, m_board.x_board);
				if (is_o_turn) {
					return o_wins;
				} else {
					return x_wins;
				}
			}
			++move_count;
		}
		print_board(m_board.o_board, m_board.x_board);
		return tie;
	}


private:


	AI1 m_o_player;
	AI2 m_x_player;
	int move_count = 0;
	tictactoe_board m_board;
};

template<typename AI1, typename AI2>
tic_tac_toe_game(AI1, AI2)->tic_tac_toe_game<AI1, AI2>;


struct console_player {
	int next_move(tictactoe_board board) {
		int r;
		std::cin >> r;
		return r;
	}
};

constexpr int popcount(int a) {
	int r = 0;
	while (a) {
		r += a % 2;
		a >>= 1;
	}
	return r;
}

struct tictactoe_ai {
	int next_move(tictactoe_board board) {
		m_is_o_player = popcount(board.combined_board()) % 2 == 0;
		return best_move(board);
	}

	int best_move(tictactoe_board board) {
		auto y = ranges::views::iota(0, 9) | ranges::views::filter([&](int idx) {
			return (board.combined_board() & (0b1 << idx)) == 0;
		});
		auto v = ranges::views::iota(0, 9) | ranges::views::transform([&](int i) {
			if ((board.combined_board() & (0b1 << i))) {
				return 0.0;
			}

			if (m_is_o_player) {
				auto c = board;
				c.o_board |= 0b1 << i;
				auto [wins, possible_spots] = possible_wins<false, true>(c);
				std::cout << i << ' ' << wins << ' ' << possible_spots << ' ' << (double)wins / possible_spots << std::endl;
				return (double)wins / (possible_spots);
			} else {
				auto c = board;
				c.x_board |= 0b1 << i;
				auto [wins, possible_spots] = possible_wins<true, false>(c);
				std::cout << i << ' ' << wins << ' ' << possible_spots << ' ' << (double)wins / possible_spots << std::endl;
				return (double)wins / (possible_spots);
			}
		}) | ranges::to<std::vector>();

		return *ranges::max_element(y, std::less(), [&](int i) {
			return v[i];
		});;
	}

	template<bool is_o_turn, bool is_o_player>
	std::tuple<int, int> possible_wins(tictactoe_board board) {
		//win checking

		if constexpr (is_o_turn) {//x just went
			if (is_winning_board(board.x_board)) {
				return {!is_o_player, 1};
			}
		} else {//o just went
			if (is_winning_board(board.o_board)) {
				return {is_o_player, 1};
			}
		}

		if (board.is_full_board()) {
			return {0, 1};
		}


		int total_wins = 0;
		int total_possible_spots = 0;

		for (int i = 0; i < 9; ++i) {
			if (board.combined_board() & 0b1 << i) {
				continue;
			}
			auto board_copy = board;
			if constexpr (is_o_turn) {
				board_copy.o_board |= 0b1 << i;
			} else {
				board_copy.x_board |= 0b1 << i;
			}
			auto [wins, possible_spots] = possible_wins<!is_o_turn, is_o_player>(board_copy);
			total_wins += wins;
			total_possible_spots += possible_spots;
		}

		return {total_wins, total_possible_spots};

	}

private:
	bool m_is_o_player = false;
};

struct random_ai {
	int next_move(tictactoe_board) {
		return engin() % 9;
	}

	std::mt19937& engin;
};

int main() {
	std::random_device r;
	std::mt19937 engine(r());

	auto g = tic_tac_toe_game(tictactoe_ai{}, tictactoe_ai{});

	g.run();
	int qwyei = 123;

	std::cin >> qwyei;

	return 0;
}
