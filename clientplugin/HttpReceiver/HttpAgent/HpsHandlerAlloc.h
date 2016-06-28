#ifndef _HPS_HANDLER_ALLOC_H_
#define _HPS_HANDLER_ALLOC_H_

#include <boost/aligned_storage.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>

//using boost::asio::ip::tcp;
namespace sdo{
	namespace hps{
		class CHpsHandlerAllocator
		  : private boost::noncopyable
		{
		public:
		  CHpsHandlerAllocator()
		    : in_use_(false)
		  {
		  }

		  void* allocate(std::size_t size)
		  {
		    if (!in_use_ && size < storage_.size)
		    {
		      in_use_ = true;
		      return storage_.address();
		    }
		    else
		    {
		      return ::operator new(size);
		    }
		  }

		  void deallocate(void* pointer)
		  {
		    if (pointer == storage_.address())
		    {
		      in_use_ = false;
		    }
		    else
		    {
		      ::operator delete(pointer);
		    }
		  }

		private:
		  boost::aligned_storage<160> storage_;
		  bool in_use_;
		};

		template <typename Handler>
		class CHpsAllocHandler
		{
		public:
		  CHpsAllocHandler(CHpsHandlerAllocator& a, Handler h)
		    : allocator_(a),
		      handler_(h)
		  {
		  }

		  template <typename Arg1>
		  void operator()(Arg1 arg1)
		  {
		    handler_(arg1);
		  }

		  template <typename Arg1, typename Arg2>
		  void operator()(Arg1 arg1, Arg2 arg2)
		  {
		    handler_(arg1, arg2);
		  }

		  friend void* asio_handler_allocate(std::size_t size,
		      CHpsAllocHandler<Handler>* this_handler)
		  {
		    return this_handler->allocator_.allocate(size);
		  }

		  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
		      CHpsAllocHandler<Handler>* this_handler)
		  {
		    this_handler->allocator_.deallocate(pointer);
		  }

		private:
		  CHpsHandlerAllocator& allocator_;
		  Handler handler_;
		};

		template <typename Handler>
		inline CHpsAllocHandler<Handler> MakeHpsAllocHandler(
		    CHpsHandlerAllocator& a, Handler h)
		{
		  return CHpsAllocHandler<Handler>(a, h);
		}
	
	}
}
#endif

