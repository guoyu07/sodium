/**
 * Copyright (c) 2012, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_SODIUM_H_
#define _SODIUM_SODIUM_H_

#include <sodium/light_ptr.h>
#include <sodium/transaction.h>
#include <functional>
#include <boost/optional.hpp>
#include <memory>
#include <list>
#include <set>
#include <stdexcept>

#define SODIUM_CONSTANT_OPTIMIZATION

namespace sodium {
    template <class A, class P> class event;
    template <class A, class P> class behavior;
    template <class A, class P> class event_loop;
    template <class A, class P> class behavior;
    template <class A, class B, class P = def_part>
    behavior<B, P> apply(const behavior<std::function<B(const A&)>, P>& bf, const behavior<A, P>& ba);
    template <class A, class P = def_part>
    event<A, P> filter_optional(const event<boost::optional<A>, P>& input);

    namespace impl {

        class behavior_;
        class behavior_impl;

        class event_ {
        friend class behavior_;
        template <class A, class P> friend class sodium::event;
        template <class A, class P> friend class sodium::behavior;
        template <class A, class P> friend class sodium::event_loop;
        friend behavior_ switch_b(transaction_impl* trans, const behavior_& bba);
        friend behavior_impl* hold(transaction_impl* trans0, const light_ptr& initValue, const event_& input);
        template <class A, class B, class P>
        friend behavior<B, P> sodium::apply(const behavior<std::function<B(const A&)>, P>& bf, const behavior<A, P>& ba);
        template <class A, class P>
        friend event<A, P> sodium::filter_optional(const event<boost::optional<A>, P>& input);
        friend behavior_ apply(transaction_impl* trans0, const behavior_& bf, const behavior_& ba);
        friend event_ map_(const std::function<light_ptr(const light_ptr&)>& f, const event_& ev);
        friend event_ switch_e(transaction_impl* trans, const behavior_& bea);
        public:
            typedef std::function<std::function<void()>(
                transaction_impl*,
                const std::shared_ptr<impl::node>&,
                const std::function<void(transaction_impl*, const light_ptr&)>&,
                bool suppressEarlierFirings,
                const std::shared_ptr<cleaner_upper>&)> listen;
            typedef std::function<void(std::vector<light_ptr>&)> sample_now;

        public:
            listen listen_impl_;
            sample_now sample_now_;

        protected:
            std::shared_ptr<cleaner_upper> cleanerUpper;
#if defined(SODIUM_CONSTANT_OPTIMIZATION)
            bool is_never_;
#endif

        public:
            event_();
            event_(const listen& listen_impl_, const sample_now& sample_now_,
                   const std::shared_ptr<cleaner_upper>& cleanerUpper
#if defined(SODIUM_CONSTANT_OPTIMIZATION)
                    , bool is_never_
#endif
                );
            event_(const listen& listen_impl_, const sample_now& sample_now_,
                   const std::function<void()>& f
#if defined(SODIUM_CONSTANT_OPTIMIZATION)
                    , bool is_never_
#endif
                );

#if defined(SODIUM_CONSTANT_OPTIMIZATION)
            bool is_never() const { return is_never_; }
#endif

        protected:

            /*!
             * listen to events.
             */
            std::function<void()> listen_raw_(
                        transaction_impl* trans0,
                        const std::shared_ptr<impl::node>& target,
                        const std::function<void(transaction_impl*, const light_ptr&)>& handle,
                        bool suppressEarlierFirings) const;

            event_ add_cleanup_(const std::function<void()>& newCleanup) const;

            const std::shared_ptr<cleaner_upper>& get_cleaner_upper() const {
                return cleanerUpper;
            }

            behavior_ hold_(transaction_impl* trans, const light_ptr& initA) const;
            event_ once_(transaction_impl* trans) const;
            event_ merge_(transaction_impl* trans, const event_& other) const;
            event_ coalesce_(const std::function<light_ptr(const light_ptr&, const light_ptr&)>& combine) const;
            event_ last_firing_only_() const;
            event_ snapshot_(transaction_impl* trans, const behavior_& beh, const std::function<light_ptr(const light_ptr&, const light_ptr&)>& combine) const;
            event_ filter_(transaction_impl* trans, const std::function<bool(const light_ptr&)>& pred) const;
        };
        #define SODIUM_DETYPE_FUNCTION1(A,B,f) \
                   [f] (const light_ptr& a) -> light_ptr { \
                        return light_ptr::create<B>(f(*a.cast_ptr<A>(NULL))); \
                   }

        event_ map_(const std::function<light_ptr(const light_ptr&)>& f, const event_& ca);

        /*!
         * Creates an event, and a function to push a value into it.
         * Unsafe variant: Assumes 'push' is called on the partition's sequence.
         */
        std::tuple<
                event_,
                std::function<void(transaction_impl*, const light_ptr&)>,
                std::shared_ptr<node>
            > unsafe_new_event(const event_::sample_now& sample_now);
        std::tuple<
                event_,
                std::function<void(transaction_impl*, const light_ptr&)>,
                std::shared_ptr<node>
            > unsafe_new_event();

        struct coalesce_state {
            boost::optional<light_ptr> oValue;
        };

        struct behavior_impl {
            behavior_impl(const light_ptr& constant);
            behavior_impl(
                const event_& changes,
                const std::function<light_ptr()>& sample);

            event_ changes;  // Having this here allows references to behavior to keep the
                             // underlying event's cleanups alive, and provides access to the
                             // underlying event, for certain primitives.

            std::function<light_ptr()> sample;

            std::function<std::function<void()>(transaction_impl*, const std::shared_ptr<node>&,
                             const std::function<void(transaction_impl*, const light_ptr&)>&)> listen_value_raw() const;
        };

        behavior_impl* hold(transaction_impl* trans0,
                            const light_ptr& initValue,
                            const event_& input);

        struct behavior_state {
            behavior_state(const light_ptr& initA);
            ~behavior_state();
            light_ptr current;
            boost::optional<light_ptr> update;
        };

        class behavior_ {
            friend impl::event_ underlyingevent_(const behavior_& beh);
            public:
                behavior_();
                behavior_(behavior_impl* impl);
                behavior_(const std::shared_ptr<behavior_impl>& impl);
                behavior_(const light_ptr& a);
                behavior_(
                    const event_& changes,
                    const std::function<light_ptr()>& sample
                );
                std::shared_ptr<impl::behavior_impl> impl;

#if defined(SODIUM_CONSTANT_OPTIMIZATION)
                /*!
                 * For optimization, if this behavior is a constant, then return its value.
                 */
                boost::optional<light_ptr> getConstantValue() const;
#endif

#if 0
                std::function<void()> listen_value_raw(impl::transaction_impl* trans, const std::shared_ptr<impl::node>& target,
                                   const std::function<void(impl::transaction_impl*, const light_ptr&)>& handle) const {
                    return impl->listen_value_raw()(trans, target, handle);
                };
#endif

                event_ values_() const;
                const event_& changes_() const { return impl->changes; }
        };

        behavior_ map_(const std::function<light_ptr(const light_ptr&)>& f,
            const behavior_& beh);

        template <class S>
        struct collect_state {
            collect_state(const S& s) : s(s) {}
            S s;
        };
    }  // end namespace impl

    template <class A, class P>
    class event;

    /*!
     * A like an event, but it tracks the input event's current value and causes it
     * always to be output once at the beginning for each listener.
     */
    template <class A, class P = def_part>
    class behavior : public impl::behavior_ {
        template <class AA, class PP> friend class event;
        template <class AA, class PP> friend class behavior;
        template <class AA, class BB, class PP>
        friend behavior<BB, PP> apply(const behavior<std::function<BB(const AA&)>, PP>& bf, const behavior<AA, PP>& ba);
        template <class AA, class PP>
        friend behavior<AA, PP> switch_b(const behavior<behavior<AA, PP>, PP>& bba);
        private:
            behavior(const std::shared_ptr<impl::behavior_impl>& impl)
                : impl::behavior_(impl)
            {
            }

        protected:
            behavior(
                const impl::event_& changes,
                const std::function<light_ptr()>& sample
            )
                : impl::behavior_(changes, sample)
            {
            }
            behavior() {}
            behavior(const impl::behavior_& beh) : impl::behavior_(beh) {}

        public:
            /*!
             * Constant value.
             */
            behavior(const A& a)
                : impl::behavior_(light_ptr::create<A>(a))
            {
            }

            /*!
             * Sample the value of this behavior.
             */
            A sample() const {
                return *impl->sample().template cast_ptr<A>(NULL);
            }

            /*!
             * listen to the underlying event, i.e. to updates.
             */
            std::function<void()> listen_raw(impl::transaction_impl* trans, const std::shared_ptr<impl::node>& target,
                                const std::function<void(impl::transaction_impl*, const light_ptr&)>& handle) const {
                return impl->changes.listen_raw_(trans, target, handle, false);
            }

            behavior<A, P> add_cleanup(const std::function<void()>& newCleanup) const {
                return behavior<A, P>(std::shared_ptr<impl::behavior_impl>(
                        new impl::behavior_impl(impl->changes.add_cleanup_(newCleanup), impl->sample)));
            }

            /*!
             * Map a function over this behaviour to modify the output value.
             */
            template <class B>
            behavior<B, P> map(const std::function<B(const A&)>& f) const {
                return behavior<B, P>(impl::map_(SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
            }

            /*!
             * Map a function over this behaviour to modify the output value.
             *
             * g++-4.7.2 has a bug where, under a 'using namespace std' it will interpret
             * b.template map<A>(f) as if it were std::map. If you get this problem, you can
             * work around it with map_.
             */
            template <class B>
            behavior<B, P> map_(const std::function<B(const A&)>& f) const {
                return behavior<B, P>(impl::map_(SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
            }

            /*!
             * Returns an event describing the changes in a behavior.
             */
            event<A, P> changes() const {
                return event<A, P>(impl->changes);
            }

            /*!
             * Returns an event describing the value of a behavior, where there's an initial event
             * giving the current value.
             */
            event<A, P> values() const {
                return event<A, P>(values_());
            }
        
            /**
             * Transform a behavior with a generalized state loop (a mealy machine). The function
             * is passed the input and the old state and returns the new state and output value.
             */
            template <class S, class B>
            behavior<B, P> collect(
                const S& initS,
                const std::function<std::tuple<B, S>(const A&, const S&)>& f
            ) const
            {
                transaction<P> trans;
                auto ea = changes().coalesce([] (const A&, const A& snd) -> A { return snd; });
                auto za = sample();
                auto zbs = f(za, initS);
                std::shared_ptr<impl::collect_state<S>> pState(new impl::collect_state<S>(std::get<1>(zbs)));
                auto p = impl::unsafe_new_event();
                auto push = std::get<1>(p);
                auto target = std::get<2>(p);
                auto kill = changes().listen_raw_(trans.impl(), target,
                         [pState, push, f] (impl::transaction_impl* trans, const light_ptr& ptr) {
                    auto outsSt = f(*ptr.cast_ptr<A>(NULL), pState->s);
                    pState->s = std::get<1>(outsSt);
                    push(trans, light_ptr::create<B>(std::get<0>(outsSt)));
                }, false);
                return event<B, P>(std::get<0>(p).add_cleanup_(kill)).hold(std::get<0>(zbs));
            }

    };  // end class behavior

    template <class A, class P = def_part>
    class event : public impl::event_ {
        template <class AA, class PP> friend class event;
        template <class AA, class PP> friend class behavior;
        template <class AA, class PP> friend class event_sink;
        template <class AA, class PP> friend event<AA, PP> filter_optional(const event<boost::optional<AA>, PP>& input);
        template <class AA, class PP> friend event<AA, PP> switch_e(const behavior<event<AA, PP>, PP>& bea);
        template <class AA, class PP> friend event<AA, PP> split(const event<std::list<AA>, PP>& e);
        public:
            /*!
             * The 'never' event (that never fires).
             */
            event() {}
        protected:
            event(const listen& listen) : impl::event_(listen) {}
            event(const impl::event_& ev) : impl::event_(ev) {}
        public:
            /*!
             * High-level interface to obtain an event's value.
             */
            std::function<void()> listen(std::function<void(const A&)> handle) const {
                transaction<P> trans;
                return listen_raw_(trans.impl(), std::shared_ptr<impl::node>(), [handle] (impl::transaction_impl* trans, const light_ptr& ptr) {
                    handle(*ptr.cast_ptr<A>(NULL));
                }, false);
            };
    
            /*!
             * Map a function over this event to modify the output value.
             */
            template <class B>
            event<B, P> map(const std::function<B(const A&)>& f) const {
                return event<B, P>(impl::map_(SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
            }
    
            /*!
             * Map a function over this event to modify the output value.
             *
             * g++-4.7.2 has a bug where, under a 'using namespace std' it will interpret
             * b.template map<A>(f) as if it were std::map. If you get this problem, you can
             * work around it with map_.
             */
            template <class B>
            event<B, P> map_(const std::function<B(const A&)>& f) const {
                return event<B, P>(impl::map_(SODIUM_DETYPE_FUNCTION1(A,B,f), *this));
            }

            event<A, P> merge(const event<A, P>& other) const {
                transaction<P> trans;
                return event<A, P>(merge_(trans.impl(), other));
            }

            /*!
             * If there's more than one firing in a single transaction, combine them into
             * one using the specified combining function.
             *
             * If the event firings are ordered, then the first will appear at the left
             * input of the combining function. In most common cases it's best not to
             * make any assumptions about the ordering, and the combining function would
             * ideally be commutative.
             */
            event<A, P> coalesce(const std::function<A(const A&, const A&)>& combine) const
            {
                return event<A, P>(coalesce_([combine] (const light_ptr& a, const light_ptr& b) -> light_ptr {
                    return light_ptr::create<A>(combine(*a.cast_ptr<A>(NULL), *b.cast_ptr<A>(NULL)));
                }));
            }

            /*!
             * Merge two streams of events of the same type, combining simultaneous
             * event occurrences.
             *
             * In the case where multiple event occurrences are simultaneous (i.e. all
             * within the same transaction), they are combined using the same logic as
             * 'coalesce'.
             */
            event<A, P> merge(const event<A, P>& other, const std::function<A(const A&, const A&)>& combine) const
            {
                return merge(other).coalesce(combine);
            }

            /*!
             * Filter this event based on the specified predicate, passing through values
             * where the predicate returns true.
             */
            event<A, P> filter(const std::function<bool(const A&)>& pred) const
            {
                transaction<P> trans;
                return event<A, P>(filter_(trans.impl(), [pred] (const light_ptr& a) {
                    return pred(*a.cast_ptr<A>(NULL));
                }));
            }

            behavior<A, P> hold(const A& initA) const
            {
                transaction<P> trans;
                return behavior<A, P>(hold_(trans.impl(), light_ptr::create<A>(initA)));
            }

            /*!
             * Sample the behavior's value as at the transaction before the
             * current one, i.e. no changes from the current transaction are
             * taken.
             */
            template <class B, class C>
            event<C, P> snapshot(const behavior<B, P>& beh, const std::function<C(const A&, const B&)>& combine) const
            {
                transaction<P> trans;
                return event<C, P>(snapshot_(trans.impl(), beh, [combine] (const light_ptr& a, const light_ptr& b) -> light_ptr {
                    return light_ptr::create<C>(combine(*a.cast_ptr<A>(NULL), *b.cast_ptr<B>(NULL)));
                }));
            }

            /*!
             * Sample the behavior's value as at the transaction before the
             * current one, i.e. no changes from the current transaction are
             * taken.
             */
            template <class B>
            event<B, P> snapshot(const behavior<B, P>& beh) const
            {
                return snapshot<B, B>(beh, [] (const A&, const B& b) { return b; });
            }

            /*!
             * Allow events through only when the behavior's value is true.
             */
            event<A, P> gate(const behavior<bool, P>& g) const
            {
                transaction<P> trans;
                return filter_optional<A>(snapshot<bool, boost::optional<A>>(
                    g,
                    [] (const A& a, const bool& gated) {
                        return gated ? boost::optional<A>(a) : boost::optional<A>();
                    })
                );
            }

            /*!
             * Adapt an event to a new event statefully.  Always outputs one output for each
             * input.
             */
            template <class S, class B>
            event<B, P> collect(
                const S& initS,
                const std::function<std::tuple<B, S>(const A&, const S&)>& f
            ) const
            {
                transaction<P> trans;
                std::shared_ptr<impl::collect_state<S>> pState(new impl::collect_state<S>(initS));
                auto p = impl::unsafe_new_event();
                auto push = std::get<1>(p);
                auto target = std::get<2>(p);
                auto kill = listen_raw_(trans.impl(), target,
                         [pState, push, f] (impl::transaction_impl* trans, const light_ptr& ptr) {
                    auto outsSt = f(*ptr.cast_ptr<A>(NULL), pState->s);
                    pState->s = std::get<1>(outsSt);
                    push(trans, light_ptr::create<B>(std::get<0>(outsSt)));
                }, false);
                return std::get<0>(p).add_cleanup_(kill);
            }

            template <class B>
            behavior<B, P> accum(
                const B& initB,
                const std::function<B(const A&, const B&)>& f
            ) const
            {
                transaction<P> trans;
                std::shared_ptr<impl::collect_state<B>> pState(new impl::collect_state<B>(initB));
                auto p = impl::unsafe_new_event();
                auto push = std::get<1>(p);
                auto target = std::get<2>(p);
                auto kill = listen_raw_(trans.impl(), target,
                         [pState, push, f] (impl::transaction_impl* trans, const light_ptr& ptr) {
                    pState->s = f(*ptr.cast_ptr<A>(NULL), pState->s);
                    push(trans, light_ptr::create<B>(pState->s));
                }, false);
                return event<B, P>(std::get<0>(p).add_cleanup_(kill)).hold(initB);
            }

            behavior<int, P> count() const
            {
                return accum<int>(0, [] (const A&, const int& total) -> int {
                    return total+1;
                });
            }

            event<A, P> once() const
            {
                transaction<P> trans;
                return event<A, P>(once_(trans.impl()));
            }

            /*!
             * Add a clean-up operation to be performed when this event is no longer
             * referenced.
             */
            event<A, P> add_cleanup(const std::function<void()>& newCleanup) const
            {
                return event<A, P>(add_cleanup_(newCleanup));
            }
    };  // end class event
    
    namespace impl {
        struct event_sink_impl {
            event_sink_impl();
            event_ construct();
            void send(transaction_impl* trans, const light_ptr& ptr) const;
            std::shared_ptr<impl::node> target;
            std::function<void(impl::transaction_impl*, const light_ptr&)> push;
        };
    }

    template <class A, class P = def_part>
    class event_sink : public event<A, P>
    {
        private:
            impl::event_sink_impl impl;
            event_sink(const impl::event_& e) : event<A, P>(e) {}

        public:
            event_sink()
            {
                *static_cast<event<A,P>*>(this) = impl.construct();
            }

            void send(const A& a) const {
                transaction<P> trans;
                impl.send(trans.impl(), light_ptr::create<A>(a));
            }
    };

    /*!
     * Make the specified event cross to partition Q.
     */
    template <class A, class P, class Q>
    event<A, Q> cross(const event<A, P>& e)
    {
        transaction<P> trans;
        event_sink<A, Q> s;
        auto kill = e.listen([s] (const A& a) {
            transaction<P> trans;
            trans.impl()->part->post([s, a] () {
                s.send(a);
            });
        });
        return s.add_cleanup(kill);
    }

    /*!
     * Make the specified behavior cross to partition Q.
     */
    template <class A, class P, class Q>
    behavior<A, Q> cross(const behavior<A, P>& b)
    {
        transaction<P> trans;
        return cross<A, P, Q>(b.changes()).hold(b.sample());
    }

    /*!
     * Filter an event of optionals, keeping only the defined values.
     */
    template <class A, class P>
    event<A, P> filter_optional(const event<boost::optional<A>, P>& input)
    {
        transaction<P> trans;
        auto p = impl::unsafe_new_event();
        auto push = std::get<1>(p);
        auto target = std::get<2>(p);
        auto kill = input.listen_raw_(trans.impl(), target,
                           [push] (impl::transaction_impl* trans, const light_ptr& poa) {
            const boost::optional<A>& oa = *poa.cast_ptr<boost::optional<A>>(NULL);
            if (oa) push(trans, light_ptr::create<A>(oa.get()));
        }, false);
        return std::get<0>(p).add_cleanup_(kill);
    }

    template <class A, class P = def_part>
    class behavior_sink : public behavior<A, P>
    {
        private:
            event_sink<A, P> e;

            behavior_sink(const behavior<A, P>& beh) : behavior<A, P>(beh) {}

        public:
            behavior_sink(const A& initA)
            {
                *dynamic_cast<behavior<A, P>*>(this) = e.hold(initA);
            }

            void send(const A& a) const
            {
                e.send(a);
            }
    };

    namespace impl {
        /*!
         * Returns an event describing the changes in a behavior.
         */
        inline impl::event_ underlyingevent_(const impl::behavior_& beh) {return beh.impl->changes;}
    };

    namespace impl {
        behavior_ apply(transaction_impl* trans, const behavior_& bf, const behavior_& ba);
    };

    template <class A, class B, class P>
    behavior<B, P> apply(const behavior<std::function<B(const A&)>, P>& bf, const behavior<A, P>& ba)
    {
        transaction<P> trans;
        return behavior<B, P>(impl::apply(
            trans.impl(),
            impl::map_([] (const light_ptr& pf) -> light_ptr {
                const std::function<B(const A&)>& f = *pf.cast_ptr<std::function<B(const A&)>>(NULL);
                return light_ptr::create<std::function<light_ptr(const light_ptr&)>>(
                        SODIUM_DETYPE_FUNCTION1(A, B, f)
                    );
            }, bf),
            ba
        ));
    }

    /*!
     * Enable the construction of event loops, like this. This gives the ability to
     * forward reference an event.
     *
     *   event_loop<A> ea;
     *   auto ea_out = doSomething(ea);
     *   ea.loop(ea_out);  // ea is now the same as ea_out
     */
    template <class A, class P = def_part>
    class event_loop : public event<A, P>
    {
        private:
            struct info {
                info(
                    const std::function<void(impl::transaction_impl*, const light_ptr&)>& pushIn,
                    const std::shared_ptr<impl::node>& target,
                    const std::shared_ptr<std::function<void()>>& pKill
                )
                : pushIn(pushIn), target(target), pKill(pKill)
                {
                }
                std::function<void(impl::transaction_impl*, const light_ptr&)> pushIn;
                std::shared_ptr<impl::node> target;
                std::shared_ptr<std::function<void()>> pKill;
            };
            std::shared_ptr<info> i;

        private:
            event_loop(const impl::event_& ev, const std::shared_ptr<info>& i) : event<A, P>(ev), i(i) {}

        public:
            event_loop()
            {
                std::shared_ptr<std::function<void()>> pKill(
                    std::shared_ptr<std::function<void()>>(
                        new std::function<void()>(
                            [] () {
                                throw std::runtime_error("event_loop not looped back");
                            }
                        )
                    )
                );
                auto p = impl::unsafe_new_event();
                *this = event_loop<A>(
                    std::get<0>(p).add_cleanup_([pKill] () {
                        std::function<void()> kill = *pKill;
                        kill();
                    }),
                    std::shared_ptr<info>(new info(std::get<1>(p), std::get<2>(p), pKill))
                );
            }

            void loop(const event<A, P>& e)
            {
                if (i) {
                    transaction<P> trans;
                    *i->pKill = e.listen_raw_(trans.impl(), i->target, i->pushIn, false);
                    i = std::shared_ptr<info>();
                }
                else
                    throw std::runtime_error("event_loop looped back more than once");
            }
    };

    /*!
     * Enable the construction of behavior loops, like this. This gives the ability to
     * forward reference a behavior.
     *
     *   behavior_loop<A> ea;
     *   auto ea_out = doSomething(ea);
     *   ea.loop(ea_out);  // ea is now the same as ea_out
     */
    template <class A, class P = def_part>
    class behavior_loop : public behavior<A, P>
    {
        private:
            event_loop<A> elp;
            std::shared_ptr<std::function<light_ptr()>> pSample;

        public:
            behavior_loop()
                : behavior<A, P>(impl::behavior_()),
                  pSample(new std::function<light_ptr()>([] () -> light_ptr {
                      throw std::runtime_error("behavior_loop sampled before it was looped");
                  }))
            {
                auto pSample = this->pSample;
                this->impl = std::shared_ptr<impl::behavior_impl>(new impl::behavior_impl(
                    elp,
                    [pSample] () { return (*pSample)(); }));
            }

            void loop(const behavior<A, P>& b)
            {
                elp.loop(b.changes());
                *pSample = b.impl->sample;
            }
    };

    namespace impl {
        event_ switch_e(transaction_impl* trans, const behavior_& bea);
    }

    /*!
     * Flatten a behavior that contains an event to give an event that reflects
     * the current state of the behavior. Note that when an event is updated,
     * due to behavior's delay semantics, event occurrences for the new
     * event won't come through until the following transaction.
     */
    template <class A, class P = def_part>
    event<A, P> switch_e(const behavior<event<A, P>, P>& bea)
    {
        transaction<P> trans;
        return event<A, P>(impl::switch_e(trans.impl(), bea));
    }

    namespace impl {
        behavior_ switch_b(transaction_impl* trans, const behavior_& bba);
    }

    /*!
     * behavior variant of switch.
     */
    template <class A, class P = def_part>
    behavior<A, P> switch_b(const behavior<behavior<A, P>, P>& bba)
    {
        transaction<P> trans;
        return behavior<A, P>(impl::switch_b(trans.impl(), bba));
    }

    template <class A, class B, class C, class P = def_part>
    behavior<C, P> lift(const std::function<C(const A&, const B&)>& f, const behavior<A, P>& ba, const behavior<B, P>& bb)
    {
        std::function<std::function<C(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<C(const B&)> {
                return [f, a] (const B& b) -> C { return f(a, b); };
            }
        );
        return apply<B, C>(ba.map_(fa), bb);
    }

    template <class A, class B, class C, class D, class P = def_part>
    behavior<D, P> lift(const std::function<D(const A&, const B&, const C&)>& f,
        const behavior<A, P>& ba,
        const behavior<B, P>& bb,
        const behavior<C, P>& bc
    )
    {
        std::function<std::function<std::function<D(const C&)>(const B&)>(const A&)> fa(
            [f] (const A& a) -> std::function<std::function<D(const C&)>(const B&)> {
                return [f, a] (const B& b) -> std::function<D(const C&)> {
                    return [f, a, b] (const C& c) -> D {
                        return f(a,b,c);
                    };
                };
            }
        );
        return apply(apply(ba.map_(fa), bb), bc);
    }

    /*!
     * Take each list item and put it into a new transaction of its own.
     *
     * An example use case of this might be a situation where we are splitting
     * a block of input data into frames. We obviously want each frame to have
     * its own transaction so that state is correctly updated after each frame.
     */
    template <class A, class P = def_part>
    event<A, P> split(const event<std::list<A>, P>& e)
    {
        event_sink<A, P> out;
        transaction<P> trans;
        auto kill = e.listen_raw_(trans.impl(), std::shared_ptr<impl::node>(),
            [out] (impl::transaction_impl* trans, const light_ptr& ptr) {
                const std::list<A>& la = *ptr.cast_ptr<std::list<A>>(NULL);
                trans->part->post([la, out] () {
                    for (auto it = la.begin(); it != la.end(); ++it)
                        out.send(*it);
                });
            }, false);
        return out.add_cleanup_(kill);
    }

}  // end namespace sodium
#endif
