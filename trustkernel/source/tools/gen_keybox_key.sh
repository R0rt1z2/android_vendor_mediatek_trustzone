#!/bin/bash
if [ $# -lt 5 ];then
    echo "$0 [ODM] [project name] [brand] [model] [platform]"
    exit 1
fi
bmp="${3}-${4}-${5}"
echo "$bmp"
DIR="$1/$2/${bmp}"
mkdir -pv "${DIR}"
CUR_DIR=${pwd}
cd "${DIR}"
echo "generate keybox aes pkkb ..."
openssl rand -hex 128 > pkkb
sed 's/^/00&/' pkkb > keybox_pkkb
rm pkkb

echo "generate keybox aes pkkb iv ..."
openssl rand -hex 32 > keybox_pkkb_iv

echo "generate keybox rsa key ..."
openssl genrsa -out kkb_pri.pem 2048
openssl rsa -inform  PEM -in kkb_pri.pem -outform DER -out kkb_pri
#openssl rsa -text -in kkb_pri.pem -pubout > tmp.txt
openssl rsa -in kkb_pri.pem  -pubout -out pubkey.pem
openssl rsa -pubin -in pubkey.pem -text -modulus > tmp.txt

sed '1,2d' tmp.txt > 1.txt
head -n 18 1.txt > 2.txt
cat 2.txt | sed 's/^[ \t]*//g' > kkb_pub
sed -i '1,1 s/^...//' kkb_pub
echo "clean ..."
rm -rf *.txt
rm -rf *.pem
cd ${CUR_DIR}
