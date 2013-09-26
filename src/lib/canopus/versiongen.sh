#! /usr/bin/env sh
#
# expected args:
# ${git_dir} ${project_loc}
#

# we want bash
test -z ${BASH_VERSION} && exec bash $0 $@
test -z ${BASH_VERSION} && exit 3
#echo PWD: `pwd` 1>&2
#echo ARGS: $@ 1>&2

if [[ $# -lt 2 ]] ; then
    GIT_DIR=$1
    PROJECT_LOC=$1
    # CCS give incorrect $git_dir, CDT doesn't...
    test "${PROJECT_LOC: -4}" = ".git" || GIT_DIR=${PROJECT_LOC}/../../../.git
else
    GIT_DIR=$1
    PROJECT_LOC=$2
fi

OUTPUT_DIR=${PROJECT_LOC}
#/Debug
test -d ${OUTPUT_DIR} || exit 1
OUTPUT=${OUTPUT_DIR}/version.c

DATE=$(date)
GITSHA=$(git --git-dir=${GIT_DIR} rev-parse HEAD || exit 2)

(
    echo "generating ${OUTPUT} ..."
    echo
    echo "DATE  : ${DATE}"
    echo "COMMIT: ${GITSHA}"
) 1>&2

(
    echo "const char *build_git_time = \"${DATE}\";"
    echo "const char build_git_sha[] = \"${GITSHA}\";"
) > ${OUTPUT}

exit 0

