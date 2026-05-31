#!/bin/bash
set -euo pipefail

if [ "$EUID" -ne 0 ]
then
    echo "Error: please run this script as root, for example:"
    echo "sudo ./install.sh"
    exit 1
fi

if [ ! -d "html" ]
then
    echo "Error: directory 'html' not found."
    echo "Please run this script from the project root directory."
    exit 1
fi

if [ ! -f "Makefile" ] && [ ! -f "makefile" ]
then
    echo "Error: Makefile not found."
    echo "Please run this script from the project root directory."
    exit 1
fi

echo "Installing packages..."

apt update

apt install -y \
build-essential \
g++ \
make \
mariadb-server \
libmariadb-dev \
apache2 \
libapache2-mod-php \
php \
php-mysql \
php-mbstring \
rsync \
unzip \
pkg-config \
cmake \
libcurl4-openssl-dev \
libtinyxml2-dev \
git

echo "Preparing database..."

systemctl enable mariadb
systemctl start mariadb

mariadb <<'SQL'
CREATE DATABASE IF NOT EXISTS meshcore
CHARACTER SET utf8mb4
COLLATE utf8mb4_general_ci;

CREATE USER IF NOT EXISTS 'meshcore'@'localhost';

GRANT ALL PRIVILEGES ON meshcore.* TO 'meshcore'@'localhost';

FLUSH PRIVILEGES;
SQL

echo "Building software..."

make

echo "Installing web files..."

rsync -a html/ /var/www/html/

echo "Done."