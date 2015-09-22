echo "instala dependencias de compilação do llvm no ubuntu"
sudo apt-get build-dep llvm-3.3-dev 

echo "instala git e mercurial"
sudo apt-get install git mercurial tortoisehg libxml2-dev cmake

echo "baixa o llvm + clang + compiler-rt - utilizando git"
git clone http://llvm.org/git/llvm.git
cd llvm
git checkout release_33
cd -
cd llvm/tools
git clone http://llvm.org/git/clang.git
cd clang
git checkout release_33
cd -
cd ../..
cd llvm/projects
git clone http://llvm.org/git/compiler-rt.git
cd compiler-rt
git checkout release_33
cd -
cd ../..

echo "compila o llvm"
mkdir llvm-build
cd llvm-build
cmake -DLLVM_REQUIRES_RTTI=1 -DBUILD_SHARED_LIBS=ON -DCLANG_INCLUDE_TESTS=OFF -DLLVM_TARGETS_TO_BUILD=X86 ../llvm
make -j4

echo "instala o llvm"
sudo make install

echo "LLVM Instalado!"
