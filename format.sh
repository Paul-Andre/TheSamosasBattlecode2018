#!/bin/bash
#
# Automatic code-formatter.
#

# Get directory of this file.
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)"

# Default arguments.
DRY_RUN=false
CHANGES_ONLY=false
STAGING_ONLY=false
REVISION="origin/master"

function print_usage {
  echo "usage: format.sh [-h | --help] [--dry-run] [--package PACKAGE]"
  echo "                 [--staging-only | --changes-only [REV]]"
  echo
  echo "Code formatter."
  echo
  echo "optional arguments:"
  echo "  -h, --help            show this help message and exit"
  echo "  --dry-run             print diffs instead of modifying in place"
  echo "  --staging-only        only format staged files"
  echo "                        cannot be used with --changes-only"
  echo "  --changes-only [REV]  only format files changed against REV"
  echo "                        default revision REV is origin/dev"
  echo "                        cannot be used with --staging-only"
}

# Parse arguments.
while [[ "${#}" > 0 ]]; do
  case "${1}" in
    --dry-run )
      DRY_RUN=true
      ;;
    --staging-only )
      STAGING_ONLY=true
      ;;
    --changes-only )
      CHANGES_ONLY=true
      if [[ "${#}" > 1 ]]; then
        REVISION="${2}"
        shift  # skip next argument
      fi
      ;;
    -h | --help )
      print_usage
      exit 0
      ;;
    * )
      print_usage
      exit 1
      ;;
  esac

  shift
done

# Validate no mutually exclusive options are set.
if [[ "${CHANGES_ONLY}" == "true" && "${STAGING_ONLY}" == "true" ]]; then
  print_usage
  exit 2
fi

# Get (A)dded, (C)opied, (M)odified, (R)enamed, or changed (T) files
if [[ "${STAGING_ONLY}" == "true" ]]; then
  CHANGED_FILES="$(git diff --name-only --diff-filter=ACMRT --cached HEAD)"
elif [[ "${CHANGES_ONLY}" == "true" ]]; then
  CHANGED_FILES="$(git diff --name-only --diff-filter=ACMRT ${REVISION})"
fi

# Included directories.
INCLUDED_DIRECTORIES="external include lib src"

#
# Clang-Format
#

if [[ ${CHANGES_ONLY} == "true" ]]; then
  CLANG_FORMAT_FILES="$(
    echo "${CHANGED_FILES}" |
      grep \
        -e '\.h$' \
        -e '\.c$' \
        -e '\.cpp$' \
        -e '\.hpp$' \
        -e '\.tpp$' \
  )"
else
  for included_directory in ${INCLUDED_DIRECTORIES}; do
    CLANG_FORMAT_FILES="${CLANG_FORMAT_FILES} $(
      find "${DIR}/${included_directory}" \
        -iname '*.h' -o \
        -iname '*.c' -o \
        -iname '*.cpp' -o \
        -iname '*.hpp' -o \
        -iname '*.tpp'
    )"
  done
fi

if [[ -n "${CLANG_FORMAT_FILES}" ]]; then
  if [[ "${DRY_RUN}" == "true" ]]; then
    for f in ${CLANG_FORMAT_FILES}; do
      diff ${f} <(clang-format ${f})
    done
  else
    echo "${CLANG_FORMAT_FILES}" |
      xargs clang-format -i
  fi
fi

exit 0
