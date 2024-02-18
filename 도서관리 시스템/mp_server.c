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
#include "Book.h"
#include "member.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}
void readSocket(int sock, char buffer[], int bufferSize)
{
    bzero(buffer, bufferSize);
    read(sock, buffer, bufferSize);
}
void timeLog(int loginType)
{
    time_t seconds = time(NULL);
    struct tm *now = localtime(&seconds);
    printf("(%02d:%02d:%02d)  ", now->tm_hour, now->tm_min, now->tm_sec);
    if (loginType == 2)
        printf("Admin  회원 ");
    else if (loginType == 1)
        printf("일반 회원 ");
}
void bookFileLock(char fileName[], struct Book record, struct flock lock, int fd, int search, int lockType)
{
    if (lockType == 1)
        lock.l_type = F_RDLCK;
    else
        lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = search * sizeof(record);
    if (search == 0)
        lock.l_len = 0;
    else
        lock.l_len = sizeof(record);
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror(fileName);
        exit(2);
    }
}
void memberFileLock(char memberDataFile[], struct member memberRecord, struct flock lock, int fd, int search, int lockType)
{
    if (lockType == 1)
        lock.l_type = F_RDLCK;
    else
        lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = search * sizeof(memberRecord);
    if (search == 0)
        lock.l_len = 0;
    else
        lock.l_len = sizeof(memberRecord);
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror(memberDataFile);
        exit(2);
    }
}
int login(int sock, char memberfile[], struct member memberRecord, char isMine[])
{
    int n, count = 1, fd = open(memberfile, O_RDONLY);
    char buffer[256], loginID[50], loginPW[50];
    struct flock lock;
    while (count != 4)
    {
        readSocket(sock, loginID, 50);
        readSocket(sock, loginPW, 50);
        for (int i = 1;; i++)
        {
            memberFileLock(memberfile, memberRecord, lock, fd, i, 1);
            lseek(fd, i * sizeof(memberRecord), SEEK_SET);
            if (read(fd, (char *)&memberRecord, sizeof(memberRecord)) != sizeof(memberRecord))
                break;
            if (memberRecord.member_Num > 0 && strcmp(memberRecord.memberID, strtok(loginID, "\n")) == 0 && strcmp(memberRecord.memberPW, loginPW) == 0)
            {
                strcpy(isMine, memberRecord.memberID);
                if (strcmp(memberRecord.isAdmin, "Y\n") == 0)
                {
                    write(sock, "2", 1);
                    return 2;
                }
                write(sock, "1", 1);
                return 1;
            }
            lock.l_type = F_ULOCK;
            fcntl(fd, F_SETLK, &lock);
        }
        write(sock, "0", 1);
        count++;
    }
    return 0;
    close(fd);
}
void memberRegister(int sock, char memberfile[], struct member memberRecord, char isMine[])
{
    int n = 0, count = 1;
    char regi[256];
    struct flock lock;
    read(sock, regi, 256);
    char *regiID = strtok(regi, "\n");
    int fd = open(memberfile, O_RDWR);
    for (int i = 1;; i++)
    {
        memberFileLock(memberfile, memberRecord, lock, fd, i, 1);
        lseek(fd, i * sizeof(memberRecord), SEEK_SET);
        if (read(fd, (char *)&memberRecord, sizeof(memberRecord)) != sizeof(memberRecord))
            break;
        if (memberRecord.member_Num > 0 && strcmp(memberRecord.memberID, regiID) == 0)
        {
            write(sock, "1", 1);
            close(fd);
            return;
        }
        if (memberRecord.member_Num > 0 && strcmp(memberRecord.memberID, isMine) == 0)
        {
            memberRecord.member_Num = count;
            strcpy(memberRecord.memberID, isMine);
            n = 1;
            break;
        }
        count++;
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    write(sock, "2", 1);
    if (n != 1)
    {
        memberRecord.member_Num = count;
        strcpy(memberRecord.memberID, regiID);
    }
    readSocket(sock, regi, 256);
    strcpy(memberRecord.memberPW, regi);
    readSocket(sock, regi, 256);
    strcpy(memberRecord.memberName, regi);
    readSocket(sock, regi, 256);
    strcpy(memberRecord.memberPhoneNumber, regi);
    readSocket(sock, regi, 256);
    strcpy(memberRecord.memberEmail, regi);
    readSocket(sock, regi, 256);
    strcpy(memberRecord.memberBirthday, regi);
    if (n != 1)
    {
        readSocket(sock, regi, 256);
        strcpy(memberRecord.isAdmin, regi);
    }
    lseek(fd, memberRecord.member_Num * sizeof(memberRecord), SEEK_SET);
    write(fd, (char *)&memberRecord, sizeof(memberRecord));
    close(fd);
}
int loginSystem(int sock, char memberfile[], int loginType, char isMine[])
{
    int n;
    char buffer[256];
    struct member memberRecord;
    for (;;)
    {
        readSocket(sock, buffer, 256);
        if (strncmp(buffer, "0", 1) == 0)
        {
            n = write(sock, "<This connection is fininshing>\n", 31);
            break;
        }
        else if (strncmp(buffer, "1", 1) == 0)
        {
            loginType = login(sock, memberfile, memberRecord, isMine);
            if (loginType != 0)
                return loginType;
        }
        else if (strncmp(buffer, "2", 1) == 0)
        {
            memberRegister(sock, memberfile, memberRecord, "-1");
        }
    }
}
int bookDBLength(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType)
{
    int fd = open(bookFile, O_RDONLY), count = 0;
    char buffer[2];
    struct flock lock;
    for (int i = 1;; i++)
    {
        bookFileLock(bookFile, bookRecord, lock, fd, i, 1);
        lseek(fd, i * sizeof(bookRecord), SEEK_SET);
        if (read(fd, (char *)&bookRecord, sizeof(bookRecord)) != sizeof(bookRecord))
            break;
        if (((strcmp(isMine, bookRecord.isMine) == 0 && loginType == 1) || loginType == 2) && bookRecord.book_Num > 0)
        {
            count++;
        }
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    sprintf(buffer, "%d", count);
    write(sock, buffer, 2);
    close(fd);
    return count;
}
int bookNameCompare(const void *a, const void *b)
{
    return strcmp(((BK *)a)->book_Name, ((BK *)b)->book_Name);
}
int priceCompare(const void *a, const void *b)
{
    return ((BK *)a)->price > ((BK *)b)->price;
}
void writeMemberData(int sock, MB temp)
{
    char buffer[256];
    sprintf(buffer, "\n1. Num : %d\n2. ID : %s \n3. PW : %s \n4. Name : %s", temp.member_Num, temp.memberID, strtok(temp.memberPW, "\n"), temp.memberName);
    write(sock, buffer, 256);
    bzero(buffer, 256);
    sprintf(buffer, "5. PhoneNum : %s6. Email : %s7. Brithday : %s8. Admin : %s \n", temp.memberPhoneNumber, temp.memberEmail, temp.memberBirthday, temp.isAdmin);
    write(sock, buffer, 256);
}
void writeBookData(int sock, BK temp)
{
    char buffer[256];
    sprintf(buffer, "1. 도서번호 : %d\n2. 도서명 : %s \n3. 저자명 : %s \n4. 출판년월일 : %s\n", temp.book_Num, temp.book_Name, temp.writer, temp.publishing);
    write(sock, buffer, 256);
    bzero(buffer, 256);
    sprintf(buffer, "5. 가격 : %d \n6. 추천리뷰 : %s \n7. 소유자id : %s\n\n", temp.price, temp.review, temp.isMine);
    write(sock, buffer, 256);
}
void listUpBook(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType, int sortType)
{
    int fd = open(bookFile, O_RDONLY);
    char buffer[256];
    struct flock lock;
    int count = bookDBLength(sock, bookFile, bookRecord, isMine, loginType);
    BK *temp = (BK *)malloc(sizeof(BK) * count);
    int num = 0;
    timeLog(loginType);
    printf("%s님이 도서 목록을 출력하셨습니다. \n", isMine);
    for (int i = 1;; i++)
    {
        bookFileLock(bookFile, bookRecord, lock, fd, i, 1);
        lseek(fd, i * sizeof(bookRecord), SEEK_SET);
        if (read(fd, (char *)&bookRecord, sizeof(bookRecord)) != sizeof(bookRecord))
            break;
        if (((strcmp(isMine, bookRecord.isMine) == 0 && loginType == 1) || loginType == 2) && bookRecord.book_Num > 0)
        {
            temp[num] = bookRecord;
            num++;
        }
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    if (sortType == 1)
        qsort(temp, count, sizeof(BK), bookNameCompare);
    else if (sortType == 2)
        qsort(temp, count, sizeof(BK), priceCompare);
    for (int i = 0; i < count; i++)
        writeBookData(sock, temp[i]);
    free(temp);
    close(fd);
}
void addBookData(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType)
{
    int search = 0;
    char sel;
    char addData[256];
    struct flock lock;
    readSocket(sock, addData, 256);
    strtok(addData, "\n");
    search = atoi(addData);
    int fd = open(bookFile, O_RDWR);
    lseek(fd, search * sizeof(bookRecord), SEEK_SET);
    read(fd, (char *)&bookRecord, sizeof(bookRecord));
    // printf("%d  %d\n",search, bookRecord.book_Num);

    if (bookRecord.book_Num == search)
    {
        write(sock, "1", 2);
        close(fd);
        return;
    }
    write(sock, "0", 2);
    bookRecord.book_Num = search;
    bookFileLock(bookFile, bookRecord, lock, fd, bookRecord.book_Num, 2);
    readSocket(sock, addData, 256);
    char *ptr = strtok(addData, " ");
    if (ptr == NULL)
    {
        write(sock, "1", 2);
        return;
    }
    strcpy(bookRecord.book_Name, ptr);
    ptr = strtok(NULL, " ");
    if (ptr == NULL)
    {
        write(sock, "1", 2);
        return;
    }
    strcpy(bookRecord.writer, ptr);
    ptr = strtok(NULL, " ");
    if (ptr == NULL)
    {
        write(sock, "1", 2);
        return;
    }
    strcpy(bookRecord.publishing, ptr);
    ptr = strtok(NULL, " ");
    if (ptr == NULL)
    {
        write(sock, "1", 2);
        return;
    }
    int price = atoi(ptr);
    bookRecord.price = price;
    ptr = strtok(NULL, "\n");
    if (ptr == NULL)
    {
        write(sock, "1", 2);
        return;
    }
    strcpy(bookRecord.review, ptr);
    write(sock, "2", 2);
    strcpy(bookRecord.isMine, isMine);
    lseek(fd, bookRecord.book_Num * sizeof(bookRecord), SEEK_SET);
    write(fd, (char *)&bookRecord, sizeof(bookRecord));
    timeLog(loginType);
    printf("%s님이 도서 정보를 추가하셨습니다. \n", isMine);
    lock.l_type = F_ULOCK;
    fcntl(fd, F_SETLK, &lock);
    bookRecord.book_Num = 0;
    close(fd);
    readSocket(sock, addData, 256);
    if (strncmp(addData, "Y\n", 3) == 0)
    {
        addBookData(sock, bookFile, bookRecord, isMine, loginType);
    }
}
void updateBookData(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType)
{
    struct flock lock;
    char updateData[256];
    timeLog(loginType);
    read(sock, updateData, 256);
    bookRecord.book_Num = atoi(updateData);
    int fd = open(bookFile, O_RDWR);
    lseek(fd, bookRecord.book_Num * sizeof(bookRecord), SEEK_SET);
    read(fd, (char *)&bookRecord, sizeof(bookRecord));
    if (bookRecord.book_Num > 0 && (loginType == 2 || (loginType == 1 && strcmp(isMine, bookRecord.isMine) == 0)))
    {
        bookFileLock(bookFile, bookRecord, lock, fd, bookRecord.book_Num, 2);
        write(sock, "0", 1);
        bzero(updateData, 256);
        read(sock, updateData, 256);
        char *ptr = strtok(updateData, " ");
        strcpy(bookRecord.book_Name, ptr);
        ptr = strtok(NULL, " ");
        strcpy(bookRecord.writer, ptr);
        ptr = strtok(NULL, " ");
        strcpy(bookRecord.publishing, ptr);
        ptr = strtok(NULL, " ");
        int price = atoi(ptr);
        bookRecord.price = price;
        ptr = strtok(NULL, "\n");
        strcpy(bookRecord.review, ptr);
        lseek(fd, bookRecord.book_Num * sizeof(bookRecord), SEEK_SET);
        write(fd, (char *)&bookRecord, sizeof(bookRecord));
        printf("%s님이 도서 정보를 갱신하셨습니다. \n", isMine);
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    else
    {
        write(sock, "1", 1);
        printf("%s님이 도서 정보를 갱신에 실패하셨습니다. \n", isMine);
    }
    close(fd);
}
void removeBookData(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType)
{
    char number[2];
    int search;
    struct flock lock;
    read(sock, number, 2);
    search = atoi(number);
    int fd = open(bookFile, O_RDWR);
    lseek(fd, search * sizeof(bookRecord), SEEK_SET);
    read(fd, (char *)&bookRecord, sizeof(bookRecord));
    if (bookRecord.book_Num == search && (loginType == 2 || (loginType == 1 && strcmp(isMine, bookRecord.isMine) == 0)))
    {
        bookFileLock(bookFile, bookRecord, lock, fd, bookRecord.book_Num, 2);
        write(sock, "0", 1);
        bookRecord.book_Num = -1;
        lseek(fd, search * sizeof(bookRecord), SEEK_SET);
        write(fd, (char *)&bookRecord, sizeof(bookRecord));
        timeLog(loginType);
        printf("%s님이 도서 정보 삭제하셨습니다. \n", isMine);
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    else
        write(sock, "1", 1);
    close(fd);
}
void adminUpdateMemberData(int sock, char memberfile[], struct member memberRecord, int loginType, char isMine[])
{
    char buffer[256];
    int search;
    read(sock, buffer, 256);
    search = atoi(buffer);
    int fd = open(memberfile, O_RDWR);
    lseek(fd, search * sizeof(memberRecord), SEEK_SET);
    read(fd, (char *)&memberRecord, sizeof(memberRecord));
    timeLog(loginType);
    if (memberRecord.member_Num > 0)
    {
        write(sock, "1", 1);
        memberRegister(sock, memberfile, memberRecord, memberRecord.memberID);
        printf("%s님이 멤버 정보 갱신하셨습니다. \n", isMine);
        return;
    }
    write(sock, "2", 1);
    close(fd);
}
void bookSearch(int sock, char bookFile[], struct Book bookRecord, char isMine[], int loginType, int searchType)
{
    char in_buffer[256];
    struct flock lock;
    timeLog(loginType);
    read(sock, in_buffer, 256);
    int fd = open(bookFile, O_RDWR);
    for (int i = 1;; i++)
    {
        bookFileLock(bookFile, bookRecord, lock, fd, i, 1);
        lseek(fd, i * sizeof(bookRecord), SEEK_SET);
        if (read(fd, (char *)&bookRecord, sizeof(bookRecord)) != sizeof(bookRecord))
            break;
        if (bookRecord.book_Num > 0 && ((searchType == 1 && strstr(strtok(bookRecord.book_Name, "\n"), strtok(in_buffer, "\n"))) || searchType == 2 && strcmp(strtok(bookRecord.writer,"\n"), strtok(in_buffer,"\n")) == 0) && (loginType == 2 || (loginType == 1 && strcmp(isMine, bookRecord.isMine) == 0)))
        {
            write(sock, "0\n", 5);
            writeBookData(sock, bookRecord);
        }
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    write(sock, "1\n", 5);
    printf("%s님이 도서 정보 검색 하셨습니다. \n", isMine);
    close(fd);
}
void listUpMemeber(int sock, char memberfile[], struct member memberRecord, char isMine[])
{
    int fd = open(memberfile, O_RDWR);
    struct flock lock;
    timeLog(2);
    for (int i = 1;; i++)
    {
        memberFileLock(memberfile, memberRecord, lock, fd, i, 1);
        lseek(fd, i * sizeof(memberRecord), SEEK_SET);
        if (read(fd, (char *)&memberRecord, sizeof(memberRecord)) != sizeof(memberRecord))
            break;
        if (memberRecord.member_Num > 0)
        {
            write(sock, "0\n", 5);
            writeMemberData(sock, memberRecord);
        }
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    write(sock, "1\n", 5);

    printf("%s님이 멤버 정보를 출력하셨습니다. \n", isMine);
    close(fd);
}
void removeMemberData(int sock, char memberfile[], char bookFile[], struct member memberRecord, struct Book bookRecord, char isMine[])
{
    char number[5];
    int search;
    struct flock lock;
    read(sock, number, 5);
    search = atoi(number);
    int fd = open(memberfile, O_RDWR);
    lseek(fd, search * sizeof(memberRecord), SEEK_SET);
    read(fd, (char *)&memberRecord, sizeof(memberRecord));
    timeLog(2);
    if (memberRecord.member_Num == search)
    {
        int bfd = open(bookFile, O_RDONLY);
        for (int i = 1;; i++)
        {
            bookFileLock(bookFile, bookRecord, lock, fd, i, 1);
            lseek(bfd, i * sizeof(bookRecord), SEEK_SET);
            if (read(bfd, (char *)&bookRecord, sizeof(bookRecord)) != sizeof(bookRecord))
                break;
            if (strcmp(bookRecord.isMine, memberRecord.memberID) == 0)
            {
                write(sock, "2\n", 5);
                close(fd);
                close(bfd);
                return;
            }
            lock.l_type = F_ULOCK;
            fcntl(fd, F_SETLK, &lock);
        }
        close(bfd);
        memberFileLock(memberfile, memberRecord, lock, fd, memberRecord.member_Num, 2);
        write(sock, "0\n", 5);
        memberRecord.member_Num = -1;
        lseek(fd, search * sizeof(memberRecord), SEEK_SET);
        write(fd, (char *)&memberRecord, sizeof(memberRecord));
        printf("%s님이 멤버 정보 삭제하셨습니다. \n", isMine);
        lock.l_type = F_ULOCK;
        fcntl(fd, F_SETLK, &lock);
    }
    else
        write(sock, "1\n", 5);
    close(fd);
}
void BookManagementSystem(int sock, char memberfile[], char bookFile[], char isMine[], int loginType)
{
    char buffer[4];
    struct Book bookRecord;
    struct member memberRecord;
    int n = 0;
    while (1)
    {
        readSocket(sock, buffer, 4);
        if (strcmp(buffer, "") == 0)
        {
            n++;
            if (n > 3)
                error("Client NetWork Error ... ");
            continue;
        }
        if (strcmp(strtok(buffer, "\n"), "1") == 0)
            listUpBook(sock, bookFile, bookRecord, isMine, loginType, 1);
        else if (strcmp(strtok(buffer, "\n"), "2") == 0)
            listUpBook(sock, bookFile, bookRecord, isMine, loginType, 2);
        else if (strcmp(strtok(buffer, "\n"), "3") == 0)
            addBookData(sock, bookFile, bookRecord, isMine, loginType);
        else if (strcmp(strtok(buffer, "\n"), "4") == 0)
            updateBookData(sock, bookFile, bookRecord, isMine, loginType);
        else if (strcmp(strtok(buffer, "\n"), "5") == 0)
            removeBookData(sock, bookFile, bookRecord, isMine, loginType);
        else if (strcmp(strtok(buffer, "\n"), "6") == 0)
            bookSearch(sock, bookFile, bookRecord, isMine, loginType, 1);
        else if (strcmp(strtok(buffer, "\n"), "7") == 0 && loginType == 1)
            bookSearch(sock, bookFile, bookRecord, isMine, loginType, 2);
        else if (strcmp(strtok(buffer, "\n"), "7") == 0 && loginType == 2)
            listUpMemeber(sock, memberfile, memberRecord, isMine);
        else if (strcmp(strtok(buffer, "\n"), "8") == 0 && loginType == 1)
            memberRegister(sock, memberfile, memberRecord, isMine);
        else if (strcmp(strtok(buffer, "\n"), "8") == 0 && loginType == 2)
            memberRegister(sock, memberfile, memberRecord, "-1");
        else if (strcmp(strtok(buffer, "\n"), "9") == 0 && loginType == 2)
            removeMemberData(sock, memberfile, bookFile, memberRecord, bookRecord, isMine);
        else if (strcmp(strtok(buffer, "\n"), "10") == 0 && loginType == 2)
            adminUpdateMemberData(sock, memberfile, memberRecord, loginType, isMine);
        else if (strcmp(strtok(buffer, "\n"), "11") == 0 && loginType == 2)
            memberRegister(sock, memberfile, memberRecord, isMine);
        else if (strcmp(strtok(buffer, "\n"), "0") == 0)
        {
            write(sock, "<This connection is fininshing>\n", 31);
            break;
        }
    }
}
void startServer(char portNumber[], char bookFile[], char memberFile[])
{
    int sockfd, newsockfd, portno, clilen, pid, loginType;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(portNumber);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    printf("*소켓기반 Multi-user Book Management 서버가 시작되었습니다*\n\n");
    clilen = sizeof(cli_addr);
    while (1)
    {
        newsockfd = accept(sockfd,
                           (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0)
        {
            char isMine[50];
            close(sockfd);
            loginType = loginSystem(newsockfd, memberFile, loginType, isMine);
            timeLog(loginType);
            printf("%s님이 접속하셨습니다. \n", isMine);
            BookManagementSystem(newsockfd, memberFile, bookFile, isMine, loginType);
            exit(0);
        }
        else
            close(newsockfd);
    } /* end of while */
}
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "사용법 : %s PortNumber  bookfile memberfile\n", argv[0]);
        exit(2);
    }
    int fd = open(argv[2], O_RDWR | O_CREAT, 0660);
    if (fd == -1)
    {
        close(fd);
        perror(argv[2]);
        exit(2);
    }
    fd = open(argv[3], O_RDWR | O_CREAT, 0660);
    if (fd == -1)
    {
        close(fd);
        perror(argv[3]);
        exit(2);
    }
    startServer(argv[1], argv[2], argv[3]);
    return 0; /* we never get here */
}