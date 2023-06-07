
/**
 * @file blackjack.cpp
 * @author your name (you@domain.com)
 * @brief
   1. Описание
   Blackjack is a popular card game played in casinos.
   The goal is to get a hand with a value as close to 21 as possible without going over.
   The player is dealt (or draws) two cards and can choose to draw more or stop.
   The dealer is dealt two cards as well, but only one is visible to the player.
   The player wins if their hand is closer to 21 than the dealer's hand.
   But if they go over 21 they automatically lose. If the player and dealer have the same value, it's a tie.

   2. Требования
   2.1 Вопросы к системе (После прочтения описания, нужно сформулировать вопросы.
   Необходимо убедиться, что требования к системе прозрачны для заказчика и разработчика.
   Ответы на вопросы станут требованиями к системе)
   2.1.1 Как много карт в колоде? Какие карты, описание?
   2.1.2 Колода карт перемешивается после каждого раунда?
   2.1.3 Как много игроков играет в игру?
   2.1.4 Разрабатываем игру или систему подсчета?
   2.1.4 Какие правила игры?

   2.2 Требования к системе
   2.2.1 Как много карт в колоде? Какие карты, описание? -- 52 карты, каждая карта имеет масть(червы, пики, бубы,
 трефы), 13 карт каждой масти. Карты могут быть
   - тип карты простой, с номером от 2 до 10, мастью, стоимость от 2 до 10
   - тип карты (валет, дама, король), стоимость 10
   - тип карты туз, стоимость 1 или 11 в зависимости от выгоды игрока
   2.2.2 Колода карт перемешивается после каждого раунда? -- перемешивается
   2.2.3 Как много игроков играет в игру? -- Два игрока, включая дилера.
   2.2.4 Разрабатываем игру или систему подсчета? -- Нужно разработать игру для игрока, не дилера.
   2.2.5 Какие правила игры? -- Правила игры следующие:
   - Игрок играет раунд. Раунд начинается с раздачи двух карт игроку и двух дилеру. Одна карта дилера видна игроку.
   - Игрок может попросить дать еще одну карту, пока сумма очков меньше 21. Если сумма очков больше 21, то игрок
 проигрывает.
   - Дилер может добавить себе карту, пока сумма очков меньше 21 и меньше игрока. Если сумма очков больше 21, то игрок
 выигрывает.
   - Если сумма очков у дилера больше, чем у игрока и меньше 21, то дилер выигрывает.
   - Игрок делает ставку в начале игры с произвольным количеством денег
   - Игрок может поставить столько денег, сколько у него есть.
   - Дилер имеет неограниченное количество денег

   3. Дизайн
   3.1 Верхнеуровневый дизайн (Абстракция)
   3.1.1 Карта Card может иметь масть Suit и значение(стоимость) Value
   3.1.2 Колода содержит 52 карты. Может отвечать за перетасовку и выдачу карты.
   3.1.3 Игроком может быть Dealer или User. Класс Player должен быть абстрактным.
   Класс Player ответственен за вытягивание карты. Класс User может иметь баланс и может делать ставки.
   3.1.4 Класс GameRound выполняет задачу контроля игры, т.е взаимодействия игроков, колоды и контроля победителя.


 * @version 0.1
 * @date 2023-05-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
#include <unordered_map>
#include <vector>

enum class Suit { CLUBS = 0, DIAMONDS, HEARTS, SPADES };
const std::unordered_map<Suit, std::string> suit_name = {{Suit::CLUBS, "CLUBS"},
                                                         {Suit::DIAMONDS, "DIAMONDS"},
                                                         {Suit::HEARTS, "HEARTS"},
                                                         {Suit::SPADES, "SPADES"}};

struct Card {
  Suit suit;
  int value;
};

std::ostream& operator<<(std::ostream& os, const Card& card)
{
  std::string suit;
  try {
    suit = suit_name.at(card.suit);
  } catch (const std::exception& ex) {
    suit = "unknown";
  }
  os << "suit: " << suit_name.at(card.suit) << ", value: " << card.value << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<Card>& cards)
{
  for (const auto& card : cards) {
    os << card;
  }
  return os;
}

/**
 * @brief Колода карт содержит 52 карты. Колоду можно перемешать. Вытащить карту с конца.
 *
 */
class Deck {
public:
  Deck()
  {
    cards_.reserve(52);
    for (int suit = 0; suit < 4; ++suit) {
      for (int value = 1; value <= 13; value++) {
        cards_.push_back(Card{Suit(suit), std::min(value, 10)});
      }
    }
  }

  Card draw()
  {
    Card card = cards_.back();
    cards_.pop_back();
    return card;
  }

  void shuffle()
  {
    for (int i = 0; i < cards_.size(); i++) {
      int j = rand() % 51;
      std::swap(cards_[i], cards_[j]);
    }
  }

  const std::vector<Card>& cards() const { return cards_; }

private:
  std::vector<Card> cards_;
};

/**
 * @brief
 *
 */
class Player {
private:
  /**
   * @brief Решает задачу контроля карт в руке. Может добавлять карту, считать счет с учетом правил.
   *
   */
  class Hand {
  public:
    Hand() : score_(0) { cards_.reserve(2); }

    void add(const Card& card)
    {
      cards_.push_back(card);
      if (card.value == 1) {
        score_ += (score_ + 11 <= 21) ? 11 : 1;
      } else {
        score_ += card.value;
      }
    }

    int score() const { return score_; }

    const std::vector<Card>& cards() const { return cards_; }

  private:
    std::vector<Card> cards_;
    int score_;
  };

private:
  Hand* hand;

public:
  Player() { this->hand = new Hand; }

  Hand* getHand() { return this->hand; }

  void clearHand() { this->hand = new Hand(); }

  void addCard(Card card) { this->hand->add(card); }

  virtual bool makeMove() = 0;
};

/**
 * @brief The UserPlayer will have a Balance and be able to place a Bet. It will also override makeMove() to prompt the
 * user for input: returning true to draw a card and false to stop. Alternatively, we could have implemented receiving
 * user input from another class, but I wanted to illustrate that the player is responsible for making a move.
 *
 */
class UserPlayer : public Player {
private:
  int balance;

public:
  UserPlayer(int balance) : Player() { this->balance = balance; }

  int getBalance() { return balance; }

  int placeBet(int amount)
  {
    if (amount > balance) {
      throw "Insufficient funds";
    }
    balance -= amount;
    return amount;
  }

  void receiveWinnings(int amount) { balance += amount; }

  /** Overrides makeMove() in Player class */
  bool makeMove()
  {
    if (this->getHand()->score() > 21) {
      return false;
    }
    // read user input
    std::cout << "Draw card? [y/n] ";
    std::string move;
    std::cin >> move;
    return move == "y";
  }
};

class Dealer : public Player {
private:
  int targetScore;

public:
  Dealer() : Player() { this->targetScore = 17; }

  void updateTargetScore(int score) { this->targetScore = score; }

  bool makeMove() { return this->getHand()->score() < this->targetScore; }
};

/**
 * @brief The GameRound will be responsible for controlling the flow of the game. It must be provided a UserPlayer,
 * Dealer, and Deck. It will also be responsible for prompting the user for a bet amount, dealing the initial cards, and
 * cleaning up the round. We added plenty of print statements for clarity, but in a real interview you could be more
 * concise.
 *
 */
class BlackJackGame {
public:
  BlackJackGame(UserPlayer* player, Dealer* dealer, Deck* deck)
  {
    this->player = player;
    this->dealer = dealer;
    this->deck = deck;
  }

  int getBetUser()
  {
    std::cout << "Enter a bet amount: ";
    int bet;
    std::cin >> bet;
    return bet;
  }

  void dealInitialCards()
  {
    for (int i = 0; i < 2; i++) {
      player->addCard(deck->draw());
      dealer->addCard(deck->draw());
    }
    std::cout << "Player hand:" << std::endl;
    std::cout << player->getHand()->cards();
    const Card& dealerCard = dealer->getHand()->cards()[0];
    std::cout << "Dealer's first card:" << std::endl;
    std::cout << dealerCard;
  }

  void cleanupRound()
  {
    player->clearHand();
    dealer->clearHand();
    std::cout << "Player balance: " << player->getBalance() << std::endl;
  }

  void play()
  {
    while (player->getBalance() > 0) {
      deck->shuffle();

      if (player->getBalance() <= 0) {
        std::cout << "Player has no more money =)" << std::endl;
        return;
      }
      int userBet = getBetUser();
      player->placeBet(userBet);

      dealInitialCards();

      // User makes moves
      while (player->makeMove()) {
        const auto& drawnCard = deck->draw();
        std::cout << "Player draws " << drawnCard << std::endl;
        player->addCard(drawnCard);
        std::cout << "Player score: " << player->getHand()->score() << std::endl;
      }
      if (player->getHand()->score() > 21) {
        std::cout << "Player loses" << std::endl;
        cleanupRound();
        return;
      }

      // Dealer makes moves
      dealer->updateTargetScore(player->getHand()->score());
      while (dealer->makeMove()) {
        dealer->addCard(deck->draw());
      }

      // Determine winner
      int dealerScore = dealer->getHand()->score();
      int playerScore = player->getHand()->score();
      if (dealerScore > 21) {
        std::cout << "Player wins" << std::endl;
        player->receiveWinnings(userBet * 2);
      } else if (dealerScore > playerScore) {
        std::cout << "Player loses" << std::endl;
      } else {
        std::cout << "Game ends in a draw" << std::endl;
        player->receiveWinnings(userBet);
      }
      cleanupRound();
    }
  }

private:
  UserPlayer* player;
  Dealer* dealer;
  Deck* deck;
};

int main()
{
  UserPlayer* player = new UserPlayer(1000);
  Dealer* dealer = new Dealer();
  Deck* deck = new Deck();
  BlackJackGame game(player, dealer, deck);
  game.play();

  return 0;
}
