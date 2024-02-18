#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <wait.h>
#include <fcntl.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}
void writeSocket(int sockfd, char buffer[], int bufferSize)
{
    bzero(buffer, bufferSize);
    fgets(buffer, bufferSize, stdin);
    write(sockfd, buffer, strlen(buffer));
}
void readSocket(int sockfd, char buffer[], int bufferSize)
{
    bzero(buffer, bufferSize);
    read(sockfd, buffer, bufferSize);
}
void printLogin()
{
    printf("BooK Management Service\n");
    printf("1. Login\n");
    printf("2. 신규회원 가입\n");
    printf("0. Quit\n");
    printf("Choose Num  >    ");
}
int login(int sockfd)
{
    int count = 1;
    char in_buffer[400];
    char out_buffer[255];
    while (count != 4)
    {
        printf("id와 password를 입력하세요.\nid : ");
        writeSocket(sockfd, out_buffer, 255);
        printf("password : ");
        writeSocket(sockfd, out_buffer, 255);
        readSocket(sockfd, in_buffer, 400);
        if (strncmp(in_buffer, "0", 1) == 0)
        {
            printf("로그인 실패 !!! (%d/3)\n", count);
            count++;
        }
        else
        {
            printf("로그인 성공 !!! \n");
            if (strncmp(in_buffer, "1", 1) == 0)
                return 1;
            if (strncmp(in_buffer, "2", 1) == 0)
                return 2;
        }
    }
    return 0;
}
void memberRegister(int sockfd, int regiType)
{
    char in_buffer[256];
    char out_buffer[256];

    if (regiType == 1)
    {
        printf("<<신규 회원가입>>\nid : ");
        writeSocket(sockfd, out_buffer, 256);
        readSocket(sockfd, in_buffer, 256);
        if (strncmp(in_buffer, "1", 1) == 0)
        {
            printf("이미 존재하는 ID 입니다 !!\n");
            return;
        }
    }
    else if (regiType == 2)
    {
        printf("<<개인 정보 변경>>\n");
        write(sockfd, "-1", 2);
        readSocket(sockfd, in_buffer, 256);
    }
    else if (regiType == 3)
    {
        printf("수정하실 회원정보의 회원번호를 입력하시오...  ");
        writeSocket(sockfd, out_buffer, 256);
        readSocket(sockfd, in_buffer, 1);
        if (strncmp(in_buffer, "2", 1) == 0)
        {
            printf("존재하지 않은 회원번호입니다. \n\n");
            return;
        }
        write(sockfd, "-1", 2);
        readSocket(sockfd, in_buffer, 256);
    }
    printf("password : ");
    writeSocket(sockfd, out_buffer, 256);
    printf("회원 이름 : ");
    writeSocket(sockfd, out_buffer, 256);
    printf("휴대폰 번호 : ");
    writeSocket(sockfd, out_buffer, 256);
    printf("이메일 : ");
    writeSocket(sockfd, out_buffer, 256);
    printf("생년월일 (YYMMDD) : ");
    writeSocket(sockfd, out_buffer, 256);
    if (regiType == 1)
    {
        printf("admin 여부 (Y/N) : ");
        writeSocket(sockfd, out_buffer, 256);
    }
    else
        printf("수정완료 !\n\n");
}
int loginSystem(int sockfd, int loginType)
{
    char in_buffer[256];
    char out_buffer[256];
    while (1)
    {
        printLogin();
        writeSocket(sockfd, out_buffer, 256);
        if (strncmp(out_buffer, "1", 1) == 0)
        {
            loginType = login(sockfd);
            if (loginType != 0)
                return loginType;
        }
        if (strncmp(out_buffer, "2", 1) == 0)
        {
            memberRegister(sockfd, 1);
        }
        else if (strncmp(out_buffer, "0", 1) == 0)
        {
            readSocket(sockfd, in_buffer, 256);
            printf("%s", in_buffer);
            exit(0);
        }
    }
}
void printMenu(int loginType)
{
    printf("Book Management\n");
    if (loginType == 1)
        printf("<일반 회원 도서관리>\n");
    else if (loginType == 2)
        printf("<Admin 회원 도서관리>\n");
    printf("1. List up All Book (Sort by 도서명)\n");
    printf("2. List up All Book (Sort by 가격)\n");
    printf("3. Add a New Book\n");
    printf("4. Update a New Book\n");
    printf("5. Remove a Book\n");
    printf("6. 도서명으로 책 검색(minimum 2char)\n");
    if (loginType == 1)
    {
        printf("7. 저자명으로 책 검색\n");
        printf("8. 개인정보 변경\n");
    }
    else if (loginType == 2)
    {
        printf("7. 회원리스트 보기\n");
        printf("8. 신규 회원정보 삽입\n");
        printf("9. 회원정보 삭제\n");
        printf("10. 회원정보 갱신\n");
        printf("11.개인정보 변경\n");
    }
    printf("0. Quit\n");
    printf("Choose Num -->   ");
}
void listUpBook(int sockfd)
{
    char in_buffer[256];
    char count[2];
    read(sockfd, count, 2);
    int num = atoi(count);
    for (int i = 0; i < num; i++)
    {
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
    }
}
void addBookData(int sockfd)
{
    char in_buffer[256];
    char out_buffer[256];
    printf("추가 할 도서정보의 도서번호를 입력하시오 (1 이상)   ");
    fgets(out_buffer, 256, stdin);
    if (strncmp(out_buffer, "0", 1) <= 0)
    {
        printf("Error ... (도서번호 1이상) \n\n");
        return;
    }
    write(sockfd, out_buffer, strlen(out_buffer));
    readSocket(sockfd, in_buffer, 256);
    if (strncmp(in_buffer, "1", 1) == 0)
    {
        printf("Error ... (중복 데이터)\n\n");
        return;
    }
    printf("도서명, 저자명, 출판년월일, 가격, 추천리뷰(공백포함) \n");
    writeSocket(sockfd, out_buffer, 256);
    readSocket(sockfd, in_buffer, 256);
    if (strncmp(in_buffer, "1", 1) == 0)
    {
        printf("데이터를 제대로 입력하세요.... 메인메뉴로 돌아갑니다...\n\n");
        return;
    }
    printf("데이터를 추가적으로 입력하시겠습니까 ?  (Y N)  ");
    writeSocket(sockfd, out_buffer, 256);
    if (strncmp(out_buffer, "Y", 1) == 0)
    {
        addBookData(sockfd);
    }
}
void updateBookData(int sockfd)
{
    char in_buffer[256];
    char out_buffer[256];
    printf("수정할 도서정보의 도서번호를 입력하시오  ");
    writeSocket(sockfd, out_buffer, 256);
    readSocket(sockfd, in_buffer, 256);
    if (strncmp(in_buffer, "1", 1) == 0)
    {
        printf("존재하지 않는 데이터입니다 !!! \n\n");
        return;
    }
    printf("도서명, 저자명, 출판년월일, 가격, 추천리뷰(공백포함) \n");
    writeSocket(sockfd, out_buffer, 256);
}
void removeData(int sockfd, int removeType)
{
    char in_buffer[3];
    char out_buffer[5];
    int number;
    if (removeType == 1)
        printf("삭제할 도서정보의 도서번호를 입력하시오  ");
    else if (removeType == 2)
        printf("삭제할 회원의 회원번호를 입력하시오  ");
    writeSocket(sockfd, out_buffer, 5);
    readSocket(sockfd, in_buffer, 5);
    if (strcmp(strtok(in_buffer, "\n"), "1") == 0)
        printf("존재하지 않는 데이터입니다 !!! \n\n");
    else if (strcmp(strtok(in_buffer, "\n"), "2") == 0)
        printf("해당 회원의 도서정보가 삭제되지 않았습니다 ! \n\n");
    else
        printf("삭제 완료 !!! \n");
}
void bookSearch(int sockfd, int serachType)
{
    char in_buffer[5];
    char out_buffer[256];
    if (serachType == 1)
        printf("검색할 도서정보의 도서제목을 입력하시오  (최소 2자)  ");
    else if (serachType == 2)
        printf("검색할 도서정보의 저자명을 입력하시오  ");
    writeSocket(sockfd, out_buffer, 256);
    while (1)
    {
        readSocket(sockfd, in_buffer, 5);
        if (strcmp(in_buffer, "1\n") == 0)
        {
            return;
        }
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
    }
}
void listUpMemeber(int sockfd)
{
    char in_buffer[256];
    char check_buffer[3];
    while (1)
    {
        readSocket(sockfd, check_buffer, 5);
        if (strcmp(strtok(check_buffer, "\n"), "1") == 0)
        {
            return;
        }
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
        readSocket(sockfd, in_buffer, 256);
        printf("%s", in_buffer);
    }
}
void bookManagementSystem(int sockfd, int loginType)
{
    char in_buffer[31];
    char out_buffer[4];
    while (1)
    {
        printMenu(loginType);
        bzero(out_buffer, 4);
        fgets(out_buffer, 4, stdin);
        write(sockfd, out_buffer, strlen(out_buffer));
        if (strcmp(strtok(out_buffer, "\n"), "1") == 0 || strcmp(strtok(out_buffer, "\n"), "2") == 0)
            listUpBook(sockfd);
        else if (strcmp(strtok(out_buffer, "\n"), "3") == 0)
            addBookData(sockfd);
        else if (strcmp(strtok(out_buffer, "\n"), "4") == 0)
            updateBookData(sockfd);
        else if (strcmp(strtok(out_buffer, "\n"), "5") == 0)
            removeData(sockfd, 1);
        else if (strcmp(strtok(out_buffer, "\n"), "6") == 0)
            bookSearch(sockfd, 1);
        else if (strcmp(strtok(out_buffer, "\n"), "7") == 0 && loginType == 1)
            bookSearch(sockfd, 2);
        else if (strcmp(strtok(out_buffer, "\n"), "7") == 0 && loginType == 2)
            listUpMemeber(sockfd);
        else if (strcmp(strtok(out_buffer, "\n"), "8") == 0 && loginType == 1)
            memberRegister(sockfd, 2);
        else if (strcmp(strtok(out_buffer, "\n"), "8") == 0 && loginType == 2)
            memberRegister(sockfd, 1);
        else if (strcmp(strtok(out_buffer, "\n"), "9") == 0 && loginType == 2)
            removeData(sockfd, 2);
        else if (strcmp(strtok(out_buffer, "\n"), "10") == 0 && loginType == 2)
            memberRegister(sockfd, 3);
        else if (strcmp(strtok(out_buffer, "\n"), "11") == 0 && loginType == 2)
            memberRegister(sockfd, 2);
        else if (strcmp(strtok(out_buffer, "\n"), "12") == 0 && loginType == 2)
        {
            read(sockfd, in_buffer, 256);
            printf("%sd\n\n", in_buffer);
        }
        else if (strcmp(strtok(out_buffer, "\n"), "0") == 0)
        {
            read(sockfd, in_buffer, 31);
            printf("%s\n", in_buffer);
            exit(0);
        }
    }
}
void startSystem(int sockfd)
{
    int loginType = loginSystem(sockfd, loginType);
    bookManagementSystem(sockfd, loginType);
}
void startClient(char ip[], char portNum[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = atoi(portNum);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(ip);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    startSystem(sockfd);
}
int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    startClient(argv[1], argv[2]);
    return 0;
}
