# Script to install needed stuff to compile metamod addon

# docker pull registry.gitlab.steamos.cloud/steamrt/scout/sdk
# docker run -v D:\repos:/dockerVolume -it <imageID>

export HL2SDKCS2='/dockerVolume/mm-cs2-scrim/sdk'
export MMSOURCE_DEV='/dockerVolume/metamod-source'
export CC=clang
export CXX=clang++

clear

echo --------------------------------------------------------------------------
echo Starting steam sniper compilator for metamod
echo

if [ "$(which "ambuild")" != "" ]; 
then
    echo "AMBUILD installed"
else
    echo "Installing ambuild"
    
    apt-get update
    apt-get install python3-setuptools -y
    apt-get install clang -y
    apt-get install python3 -y
    apt-get install gcc -y

    cd "/dockerVolume/ambuild" && python3 setup.py install

    echo "AMBUILD Installed"
fi

cd "/dockerVolume/s2_sample_mm"
mkdir build
cd build

#python ../configure.py --enable-optimize --symbol-files --sdks cs2
python ../configure.py --sdks cs2 --targets x86_64
ambuild


# Copy files to the deploy folder
todaysDate=$(date +%d-%b-%H_%M)
#mkdir ../../dockerVolume/$todaysDate
#DEPLOY_FOLDER=../../dockerVolume/$todaysDate

echo finished
echo $todaysDate