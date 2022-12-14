#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Количество номеров - оно фиксировано по условию задачи
#define ROOMS_AMOUNT 25

// Прибыль гостиницы
unsigned long long income = 0;

// Структура, описывающая номер
typedef struct {
    // Номер комнаты
    int number;
    // Стоимость номера
    int cost;
    // Переменная, показывающая, занята комната или нет
    int is_taken;
    // Мьютекс, в данном случае "ключ". Клиент всегда закрывает номер и никого не пускает при наличии такой возможности
    pthread_mutex_t mutex;
} Room;

// Гостиница (в данном варианте состоит из номеров)
Room hotel[ROOMS_AMOUNT];

// Функция, производящая подсчет количества свободных номеров для клиента.
int getAmountOfAvailableRooms(int money) {
    int available_rooms_amount = 0;
    for (int i = 0; i < ROOMS_AMOUNT; ++i) {
        // Если у клиента есть деньги хотя бы на день и номер не занят, то у клиента становится на один вариант выбора жилья больше.
        if (money >= hotel[i].cost && hotel[i].is_taken == 0) {
            ++available_rooms_amount;
        }
    }
    // Возврат количества номеров.
    return available_rooms_amount;
}

// Функция описывает, какие комнаты предлагаются клиенту
void fillAvailableRooms(Room *available_rooms, int money) {
    int current_room = 0;
    for (int i = 0; i < ROOMS_AMOUNT; ++i) {
        // Если у клиента есть деньги хотя бы на день и номер не занят, то этот вариант добавляется в возможные комнаты клиента
        if (money >= hotel[i].cost && hotel[i].is_taken == 0) {
            available_rooms[current_room] = hotel[i];
            ++current_room;
        }
    }
}

// Функция, описывающая процесс проживания в номере.
void rent(int client_number, int room_index, int money) {
    // Сообщается, что конкретный клиент занял конкретный номер. Оговаривается цена
    printf("Client with number %d choose %d room. Cost per day: %d rubles\n", client_number, hotel[room_index].number, hotel[room_index].cost);
    int days = 1;
    // Клиент закрывает номер от других клиентов.
    // Может произойти такое, что номер не был занят и два клиента (или больше) приметили его
    // Первый, кто добрался до ключа, закрывает номер и живет в свое удовольствие.
    pthread_mutex_lock(&hotel[room_index].mutex);
    // Теперь номер занят, об этом знают остальные
    hotel[room_index].is_taken = 1;
    // Клиент живет, пока его денег хватает на оплату номера за день
    while (money >= hotel[room_index].cost) {
        sleep(2);
        // Сообщается, сколько дней он уже прожил в данном номере
        printf("Client with number %d live in %d room for %d day(s)\n", client_number, hotel[room_index].number, days);
        ++days;
        // Клиент платит гостинице
        money -= hotel[room_index].cost;
        income += hotel[room_index].cost;
    }
    // У клиента закончились деньги - он выселяется, отпирает номер. Следующий, кто захочет в этом номере жить и получит ключ, будет в нем жить
    pthread_mutex_unlock(&hotel[room_index].mutex);
    // Комната официальна свободна для проживания
    hotel[room_index].is_taken = 0;
}


// Функция описывает то, как клиент ищет свободный номер
void *takeRoom(void *args) {
    sleep(1);
    int *client_number = (int *)(args);
    // Клиент получает случайную сумму денег (в пределах 18000 рублей)
    int money = rand() % 18001;
    // Сообщается номер клиента и его баланс
    printf("\nClient with number %d has %d rubles on his balance\n", *client_number, money);
    // Переменная, хранящая в себе количество доступных номеров для данного клиента
    int available_rooms_amount = getAmountOfAvailableRooms(money);
    // Полный список свободных номеров
    Room available_rooms[available_rooms_amount];
    // Предоставление информации о номерах
    fillAvailableRooms(available_rooms, money);
    // Если клиенту доступен хотя бы один номер
    if (available_rooms_amount != 0) {
        // Он выбирает случайный из свободных
        int room_index = rand() % available_rooms_amount;
        // Совершает оплату, "владеет" номеров
        rent(*client_number, room_index, money);
        // Покидает отель при нехватке денег. Сообщается, что комната освободилась
        printf("Client with number %d left the hotel\n", *client_number);
        printf("Now room with number %d is free\n\n", room_index + 1);
    } else {
        // Если свободных номеров нет или у клиента не хватает денег на проживание в самом дешевом номере на текущий момент, то он покидает гостиницу
        printf("Client with number %d can't stay in this hotel! ", *client_number);
        printf("Client with number %d left the hotel\n\n", *client_number);
    }
    return NULL;
}

// Функция, описываюшая подготовку номеров
void prepareRooms() {
    for (int i = 0; i < ROOMS_AMOUNT; ++i) {
        // Устанавливается цена в соответствии с условием
        // Индекс меньше 10 - цена 2000
        if (i < 10) {
            hotel[i].cost = 2000;
        } else if (i < 20) {
            // Индекс меньше 20 - цена 4000
            hotel[i].cost = 4000;
        } else {
            // Иначе - цена 6000
            hotel[i].cost = 6000;
        }
        // Номер комнаты равен индексу + 1
        hotel[i].number = i + 1;
        // По умолчанию комнаты не заняты (0 - комната свободна, 1 - занята)
        hotel[i].is_taken = 0;
        // Инициализация мьютексов (ключи вешаются на стойку)
        pthread_mutex_init(&(hotel[i].mutex), NULL);
    }
}


// Точка входа в программу.
int main(int argc, char **argv) {
    // Переменная, отвечающая за количество клиентов.
    int clients_amount;
    // Проверка на некорректный ввод
    if (argc != 2) {
        printf("Error: found %d arguments. Needs exactly 2\n", argc);
        return -1;
    }
    clients_amount = atoi(argv[1]);
    // Проверка на число клиентов, не входящий в допустимый диапазон
    if (clients_amount < 5 || clients_amount > 50) {
        printf("Error: Clients amount can't be less than 5 or greater than 50\n");
        return -1;
    }
    // Установка сида для генерации псевдослучайных чисел.
    srand(time(NULL));
    // Работа гостиницы начинается с подготовки номеров, устанавливается стоимость, крепятся таблички с номерами
    prepareRooms();
    // Приходят клиенты
    pthread_t clients[clients_amount];

    int clients_numbers[clients_amount];

    // Каждый клиент помечается конкретным номером
    for (int i = 0; i < clients_amount; ++i) {
        clients_numbers[i] = i + 1;
    }

    // Клиент заходит в гостиницу и ищет свободный номер
    for (int i = 0; i < clients_amount; i++) {
        pthread_create(&clients[i], NULL, takeRoom, &clients_numbers[i]);
    }

    // Все клиенты побывали в гостинице - кто-то ушел без ночлега, кому-то повезло больше (клиенты уходят, не имея претензий)
    for (int i = 0; i < clients_amount; i++) {
        pthread_join(clients[i], NULL);
    }

    // Ключи убирают со стойки с ключами
    for (int i = 0; i < ROOMS_AMOUNT; ++i) {
        pthread_mutex_destroy(&(hotel[i].mutex));
    }
    
    // Сообщения о том, что клиенты ушли, вывод прибыли гостиницы.
    printf("\nAll clients left the hotel!\n");
    printf("\nHotel income: %lld rubles\n", income);
    return 0;
}