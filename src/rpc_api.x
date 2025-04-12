struct Coord {
    int x;
    int y;
};

struct request_t {
    int op;
    int key;
    char value1<256>;
    int N_value2;
    double V_value2<32>;
    struct Coord value3;
};

struct response_t {
    int result;
    char value1<256>;
    int N_value2;
    double V_value2<32>;
    struct Coord value3;
};

program KEYS {
    version VKEYS {
        int DESTROY(void) = 0;
        response_t SET_VALUE(request_t request) = 1;
        response_t GET_VALUE(request_t request) = 2;
        response_t MODIFY_VALUE(request_t request) = 3;
        response_t DELETE_KEY(request_t request) = 4;
        response_t EXIST(request_t request) = 5;
    } = 1;
} = 1;