#include <functional>
#include <memory>
#include <vector>
#include <iostream>
#include <tuple>

namespace mmc {

struct base_callback
{
    virtual void do_execute(int n) const noexcept = 0;
    virtual ~base_callback() = default;

    using ptr = std::unique_ptr<base_callback>;
};

template<typename FUNC, typename... ARGS>
struct CallBack : public base_callback
{
    using func_t = FUNC;
    using args_t = std::tuple<ARGS...>;
    static constexpr auto size = std::tuple_size<args_t>::value;
    static constexpr auto indezes = std::make_index_sequence<size>{};

    func_t ivFunction;
    args_t ivArgs;

    explicit CallBack(FUNC&& function, ARGS&&... args) :
            ivFunction(std::forward<FUNC>(function)),
            ivArgs( std::forward<ARGS>(args)... )
    {}

    template<size_t ... I>
    void call(int n, std::index_sequence<I ...>) const
    {
        std::invoke(ivFunction, std::get<I>(ivArgs)..., n);
    }

    void do_execute(int n) const noexcept override
    {
        call(n, indezes);
    }
};

template<typename FUNC, typename... ARGS>
inline auto createCallBack(FUNC&& func, ARGS&&... args)
{
    return std::make_unique< CallBack<FUNC, ARGS...> >
            (std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

//-------------------------------------------------------------------
struct Subscription
{
    int                ivFilter;
    base_callback::ptr ivCallback;

    explicit Subscription(int number, base_callback::ptr&& callback) :
            ivFilter(number),
            ivCallback(std::move(callback))
    {}

    void call(int n) const noexcept
    {
        if (ivCallback)
        {
            ivCallback->do_execute(n);
        }
    }

    bool matches(int filter) const
    { return filter == ivFilter; }
};

struct Subscriptions
{
    using Container = std::vector<Subscription>;

    Container ivSubscriptions;

    void call(int n) const noexcept
    {
        for(auto& sub : ivSubscriptions)
        {
            if (sub.matches(n))
            { sub.call(n); }
        }
    }

    template<typename FUNC, typename... ARGS>
    void add(int filter, FUNC&& func, ARGS&&... args)
    {
        ivSubscriptions.emplace_back(filter,
                                     createCallBack(std::forward<FUNC>(func), std::forward<ARGS>(args)...));
    }
};
}

static void print(const std::string& name, int n)
{
    std::cout << name << " : " << n << std::endl;
}

struct StringPrinter
{
    std::string ivText;

    void print(int add, int number) const
    {
        std::cout << ivText << " : " << add + number << std::endl;
    }

    void operator () (int number) const
    {
        print(0, number);
    }

    StringPrinter(const StringPrinter&) = delete;
    StringPrinter(StringPrinter&&) = delete;
    StringPrinter& operator = (const StringPrinter&) = delete;
    StringPrinter& operator = (StringPrinter&&) = delete;
};

int main(int /*argc*/, const char* /*argv*/[])
{
    mmc::Subscriptions subs;

    subs.add(42, print, "Hello");
    subs.add(42, print, "World");

    subs.add(42, [](int number){ std::cout << "lambda " << number << std::endl; });

    StringPrinter printer{"EMPTY"};
    subs.add(42, printer);
    subs.add(42, &StringPrinter::print, &printer, 1);
    subs.add(42, &StringPrinter::print, &printer, 2);
    printer.ivText = "Something";

    subs.call(42);

    return 0;
}
