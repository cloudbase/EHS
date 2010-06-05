#include <exception>
#include <string>

namespace tracing {

    class bfd_tracer
    {
        public:
            bfd_tracer(int _maxframes);

            ~bfd_tracer();

            std::string trace(int skip) const;

            bfd_tracer(const bfd_tracer &);

            bfd_tracer & operator = (const bfd_tracer &);

        private:
            int maxframes;
            int frames;
            void **tbuf;
    };

    /**
     * An exception, which can generate a backtrace
     */
    class exception : public std::exception
    {
        public:
            /**
             * Constructs a new backtrace_exception.
             */
            exception() throw();

            virtual ~exception() throw();

            /**
             * Returns a backtrace.
             * @return string with multiple backtrace lines.
             */
            virtual const char* where() const throw();

        private:
            bfd_tracer tracer;
    };

    class runtime_error : public exception
    {
        public:
            explicit runtime_error(const std::string& __arg)
                : exception(), msg(__arg) { }
            virtual ~runtime_error() throw() { }
            virtual const char* what() const throw()
            { return msg.c_str(); }

        private:
            std::string msg;
    };
}
