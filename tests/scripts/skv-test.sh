#!/bin/bash

function write_script
{
DIR_NAME=$(printf 'skv-N%04d-D%05d' ${NUM_SERVERS} ${QDEPTH})

echo "Creating job $DIR_NAME"

mkdir -p $DIR_NAME

cat << _EOF_ > ${DIR_NAME}/submit-job.bash
#!/bin/bash

pdsh -w bbpbgas[049-064] 'rm -rf /ext4/*'

${MPIEXEC} -n ${NUM_SERVERS} ${EXECUTABLE1} &

sleep 5

/gpfs/bbp.cscs.ch/home/biddisco/gcc/bgas/build/bgas/Lunchbox/tests/perf_persistentMap ${PROGRAM_PARAMS} > ${DIR_NAME}.csv

pdsh -w bbpbgas[049-064] 'pkill -9 SKVServer'
 
_EOF_

chmod 775 ${DIR_NAME}/submit-job.bash

echo "cd ${DIR_NAME}; ./submit-job.bash; cd \$BASEDIR" >> run_jobs.bash

}

# get the path to this generate script, works for most cases
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null
echo "Generating jobs using base directory $BASEDIR"

# Create another script to submit all generated jobs to the scheduler
echo "#!/bin/bash" > run_jobs.bash
echo "BASEDIR=$BASEDIR" >> run_jobs.bash
echo "cd $BASEDIR" >> run_jobs.bash
chmod 775 run_jobs.bash

#
# 
#
MPIEXEC="mpiexec"
QUEUE=benchmark
EXECUTABLE1=/gpfs/bbp.cscs.ch/home/biddisco/gcc/bgas/build/bgas/bin/SKVServer
LIB_PATH="@LIB_PATH@"
JOB_OPTIONS1="@JOB_OPTIONS1@"
MEMPERNODE=
TIME="00:05:00"

# Loop through all the parameter combinations generating jobs for each
for NUM_SERVERS in 1 2 4 8 
do
  for QDEPTH in 0 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 
  do
    PROGRAM_PARAMS="${QDEPTH} ${NUM_SERVERS}"
    write_script
  done
done

