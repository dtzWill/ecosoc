echo "siot download"
git clone git@gitlab.com:pablo.marcondes/siot.git
cd  siot
#hg update testes
cd -

echo "build siot"
mkdir siot-build
cd siot-build
cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_REQUIRES_RTTI=1 -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j4 -DLLVM_REQUIRES_EH=1 ../siot
make -j4

