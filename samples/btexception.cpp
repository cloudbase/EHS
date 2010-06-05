#include "btexception.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include <execinfo.h>
#include <bfd.h>

#define DMGL_PARAMS      (1 << 0)       /* Include function args */
#define DMGL_ANSI        (1 << 1)       /* Include const, volatile, etc */
#define DMGL_JAVA        (1 << 2)       /* Demangle as Java rather than C++. */
#define DMGL_VERBOSE     (1 << 3)       /* Include implementation details.  */
#define DMGL_TYPES       (1 << 4)       /* Also try to demangle type encodings.  */
#define DMGL_RET_POSTFIX (1 << 5)       /* Print function return types (when
                                           present) after function signature */

static asymbol **syms;
static bfd *abfd;

static void fetchsyms (bfd *abfd)
{
    long symcount;
    unsigned int size;
    if (0 == (bfd_get_file_flags(abfd) & HAS_SYMS))
        return;
    symcount = bfd_read_minisymbols (abfd,
            FALSE, reinterpret_cast<void **>(&syms), &size);
    if (0 == symcount)
        symcount = bfd_read_minisymbols (abfd,
                TRUE /* dynamic */, reinterpret_cast<void **>(&syms), &size);
}

typedef struct {
    bfd_vma pc;
    const char *filename;
    const char *functionname;
    unsigned int line;
    bfd_boolean found;
} bfdstate_t;

static void findit(bfd *abfd, asection *section, void *data)
{
    bfdstate_t *state = (bfdstate_t *)data;
    bfd_vma vma;
    bfd_size_type size;

    if (state->found)
        return;
    if (0 == (bfd_get_section_flags(abfd, section) & SEC_ALLOC))
        return;
    vma = bfd_get_section_vma (abfd, section);
    if (state->pc < vma)
        return;
    size = bfd_get_section_size (section);
    if (state->pc >= vma + size)
        return;
    state->found =
        bfd_find_nearest_line(abfd, section, syms, state->pc - vma,
                &state->filename, &state->functionname, &state->line);
}

static int init_backtrace()
{
    static int init = 1;
    char **matching;

    if (init) {
        init = 0;
        bfd_init();
    }
    if (!bfd_set_default_target("i686-redhat-linux-gnu"))
        return 0;
    abfd = bfd_openr ("/proc/self/exe", NULL);
    if (NULL == abfd)
        return 0;
    if (bfd_check_format(abfd, bfd_archive))
        return 0;
    if (!bfd_check_format_matches(abfd, bfd_object, &matching)) {
        return 0;
    }
    fetchsyms(abfd);
    return 1;
}

static void exit_backtrace()
{
    if (NULL != syms)
        free (syms);
    if (NULL != abfd)
        bfd_close (abfd);
    abfd = NULL;
    syms = NULL;
}

namespace tracing {

    bfd_tracer::bfd_tracer(const bfd_tracer &other) :
        maxframes(other.maxframes),
        frames(other.frames),
        tbuf(new void* [maxframes])
    {
        memcpy(tbuf, other.tbuf, sizeof(void *) * maxframes);
    }

    bfd_tracer::bfd_tracer(int _maxframes) :
        maxframes(_maxframes), frames(0), tbuf(new void* [_maxframes])
    {
        frames = backtrace(tbuf, maxframes);
    }

    bfd_tracer::~bfd_tracer()
    {
        delete [] tbuf;
        exit_backtrace();
    }

    std::string bfd_tracer::trace(int skip) const {
        std::string ret;
        if (init_backtrace()) {
            for (int i = skip; i < frames ; i++) {
                std::ostringstream oss;
                bfdstate_t state;
                state.pc = reinterpret_cast<bfd_vma>(tbuf[i]);
                state.found = FALSE;
                bfd_map_over_sections (abfd, findit, &state);
                if (! state.found) {
                    oss << std::setw(3) << i - skip << " "
                        << std::setfill('0') << std::setw(10)
                        << std::hex << std::internal
                        << tbuf[i] << " [no debug info]" << std::endl;
                } else {
                    do {
                        const char *name;
                        char *alloc = NULL;

                        name = state.functionname;
                        if (NULL == name || '\0' == *name)
                            name = "??";
                        else {
                            alloc = bfd_demangle(abfd, name,
                                    DMGL_ANSI|DMGL_PARAMS|DMGL_TYPES);
                            if (alloc != NULL)
                                name = alloc;
                        }

                        oss << std::setw(3) << i - skip << " " << name << " [";
                        if (alloc != NULL)
                            free (alloc);
                        if (state.filename) {
                            oss << state.filename << ", line " << state.line;
                        } else {
                            oss << "no debug info";
                        }
                        oss << "]" << std::endl;

                        state.found =
                            bfd_find_inliner_info(abfd,
                                    &state.filename, &state.functionname, &state.line);
                    } while (state.found);
                }
                ret.append(oss.str());
            }
        } else {
            for (int i = skip; i < frames ; i++) {
                std::ostringstream oss;
                oss << std::setw(3) << i - skip << " "
                    << std::setfill('0') << std::setw(10) << std::hex << std::internal
                    << tbuf[i] << " [no debug info]" << std::endl;
                ret.append(oss.str());
            }
        }
        return ret;
    }

    exception::exception() throw() :
        std::exception(), tracer(100)
    {
    }

    exception::~exception() throw()
    {
    }

    const char* exception::where() const throw() {
        return tracer.trace(2).c_str();
    }
}
