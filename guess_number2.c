#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

#define MSG_KEY 7815

// Структура для передачи чисел
struct msg_number {
    long mtype; 
    int number;        
};

// Структура для передачи попыток
struct msg_attempts {
    long mtype;
    int attempts;      
};

void game_round(int N, int msgid, int player_num) {
    srand(time(NULL) + getpid());
    struct msg_number secret_msg;
    struct msg_attempts attempts_msg;
    pid_t pid;

    secret_msg.number = rand() % N + 1;
    secret_msg.mtype = player_num;  
    printf("Игрок %d загадал число от 1 до %d: %d\n", player_num, N, secret_msg.number);
    msgsnd(msgid, &secret_msg, sizeof(secret_msg.number), 0);  

    attempts_msg.attempts = 0;
    attempts_msg.mtype = player_num == 1 ? 2 : 1;
    msgsnd(msgid, &attempts_msg, sizeof(attempts_msg.attempts), 0); 

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {  
        int guess;
        while (1) {
            if (msgrcv(msgid, &attempts_msg, sizeof(attempts_msg.attempts), player_num == 1 ? 2 : 1, 0) == -1) {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
            
            attempts_msg.attempts++;
            guess = rand() % N + 1;
            printf("Игрок %d предполагает: %d\n", player_num == 1 ? 2 : 1, guess);

            if (guess == secret_msg.number) {
                printf("Игрок %d: Я угадал число за %d попыток!\n", player_num == 1 ? 2 : 1, attempts_msg.attempts);
                attempts_msg.mtype = 3;
                msgsnd(msgid, &attempts_msg, sizeof(attempts_msg.attempts), 0);  
                exit(EXIT_SUCCESS);
            }

            msgsnd(msgid, &attempts_msg, sizeof(attempts_msg.attempts), 0);  
        }
    } else {
        if (msgrcv(msgid, &attempts_msg, sizeof(attempts_msg.attempts), 3, 0) == -1) {
            perror("msgrcv");
            kill(pid, SIGTERM);
            wait(NULL);
            return;
        }
        printf("Игрок %d: Игрок %d угадал число %d за %d попыток.\n", player_num, player_num == 1 ? 2 : 1, secret_msg.number, attempts_msg.attempts);
        // Сигнал завершения для дочернего процесса
        attempts_msg.mtype = 4; 
        msgsnd(msgid, &attempts_msg, sizeof(attempts_msg.attempts), 0);
        // Ожидание завершения дочернего процесса
        wait(NULL); 
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <N> <количество игр>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int N = atoi(argv[1]);
    int num_games = atoi(argv[2]);

    if (N <= 0 || num_games <= 0) {
        fprintf(stderr, "N и количество игр должны быть положительными числами.\n");
        exit(EXIT_FAILURE);
    }
    setlocale(LC_ALL, "ru_RU");

    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_games; i++) {
        printf("\n--- Игра %d: Игрок 1 загадывает, Игрок 2 угадывает ---\n", i + 1);
        game_round(N, msgid, 1);

        printf("\n--- Игра %d: Игрок 2 загадывает, Игрок 1 угадывает ---\n", i + 1);
        game_round(N, msgid, 2);
    }
    // Удаляем очередь сообщений
    msgctl(msgid, IPC_RMID, NULL); 
    printf("Все игры завершены. Очередь сообщений удалена.\n");
    return 0;
}
