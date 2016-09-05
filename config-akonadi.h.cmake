#cmakedefine01 Backtrace_FOUND
#if Backtrace_FOUND
# include <@Backtrace_HEADER@>
#endif

#cmakedefine HAVE_UNISTD_H 1

#cmakedefine HAVE_MALLOC_TRIM 1

#define AKONADI_DATABASE_BACKEND "@AKONADI_DATABASE_BACKEND@"
