#!/bin/bash

function help {
    echo "accepts the following commands"
    echo " create_containers"
    echo " run_test"
    echo " test"
    echo " clean"
    echo " help"
}

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

TESTID=${PWD##*/}

if [ -f $DIR/.env ]; then
    source $DIR/.env
fi

function create_containers {
    cp ../../build/router ../router_node/
    docker-compose build
    docker-compose up -d -t 0
}

function run_test {
    docker-compose exec pinger ping.sh > logs.log
}

function clean_up {
    docker-compose logs 2>&1 >> logs.log
    docker-compose stop -t 0

    docker-compose down
    docker-compose rm -f
}

function test {
    create_containers
    run_test
    clean_up
}

case $1 in
    "create")
        create_containers
        ;;
    "run_test")
        run_test
        ;;
    "clean")
        clean_up
        ;;
    "test")
        test
        ;;
    *)
        help
        ;;
esac
