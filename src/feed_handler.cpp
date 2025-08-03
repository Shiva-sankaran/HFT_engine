#include<network/feed_handler.h>



FeedHandler::FeedHandler(int n_threads, int n_sockets_per_thread, std::vector<MarketFeed> feeds, std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_book_queue)

    :
    _n_threads(n_threads),
    _n_sockets_per_thread(n_sockets_per_thread),
    _feeds(feeds),
    // _ip(ip),
    // _base_port(base_port),
    _symbol_to_order_book_queue_(symbol_to_order_book_queue){

    }

void FeedHandler::start(){

    for(int i = 0; i<_n_threads; i++){
        
            int epoll_fd = epoll_create1(0);
            if(epoll_fd<0){
                perror("epoll");
                exit(1);
            }

            for(int j = 0; j<_n_sockets_per_thread; j++){
                int feed_idx = i*_n_sockets_per_thread + j;
                int feed_fd = create_feed_connection(_feeds[feed_idx]);

                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = feed_fd;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, feed_fd, &ev) < 0){
                    perror("epoll_ctl");
                    exit(1);
                }
            }

            std::cout << "Connected to market feeds group " << i <<'\n';
            _epoll_fds.push_back(epoll_fd);

    }
}

void FeedHandler::listen(){
    for(auto& epoll_fd: _epoll_fds){
        _epoll_workers.emplace_back(std::thread([epoll_fd, this]{
        
        int active_fds = _n_sockets_per_thread;
        int MAX_EVENTS = 64;
        epoll_event events[MAX_EVENTS];
        char buffer[3000];

        while(active_fds>0){
            int n = epoll_wait(epoll_fd,events,MAX_EVENTS,-1);

            for(int k = 0; k<n;k++){
                int trig_fd = events[k].data.fd;

                while (true) {
                    ssize_t len = read(trig_fd, buffer, sizeof(buffer) - 1);

                    if (len == -1 && errno == EAGAIN) {
                        break; // no more data now
                    } else if (len == 0) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, trig_fd, nullptr);
                        close(trig_fd);
                        active_fds--;
                        break;
                    } else if (len < 0) {
                        perror("read error");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, trig_fd, nullptr);
                        close(trig_fd);
                        active_fds--;
                        break;
                    }

                    buffer[len] = '\0';
                    std::cout << "Data: " << buffer << "\n";
                }

            }
        }
        close(epoll_fd);

        }));

    }
}

void FeedHandler::stop(){
    std::cout << "Stopping Feed Handler" << std::endl;
    for(auto& epoll_worker: _epoll_workers){
        epoll_worker.join();
    }
}

int FeedHandler::create_feed_connection(MarketFeed feed){

    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket");
        exit(1);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(feed.port);
    inet_pton(AF_INET,feed.ip.c_str(),&addr.sin_addr.s_addr);

    fcntl(fd, F_SETFL, fcntl(fd,F_GETFL,0) | O_NONBLOCK);


    if(connect(fd,(sockaddr*)&addr,sizeof(addr)) < 0){
        if (errno != EINPROGRESS) {
            perror("connect");
            exit(1);
        }
    }

    std::cout << "Connected to market at " << feed.ip << ":" << feed.port << ":" << feed.name << "\n";
    return fd;
}