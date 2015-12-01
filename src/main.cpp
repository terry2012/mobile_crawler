#include "common.hpp"
#include "scheduler.hpp"

Scheduler<>* scheduler;

void my_handler(int s)
{
        LOGW("");
        LOGW("exiting ..");

        scheduler->end();
        exit(0);
}

int main(int argc, char **argv)
{
        signal(SIGINT, my_handler);
        signal(SIGTERM, my_handler);

        scheduler = &(Scheduler<>::get_instance(PENDING_URL_FILE, GRAPH_FILE));
        int rc = scheduler->run();
        scheduler->end();

        return rc;
}
