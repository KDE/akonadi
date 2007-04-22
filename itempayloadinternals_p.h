


struct PayloadBase
{
    virtual ~PayloadBase() { };
    virtual PayloadBase * clone() const = 0;
    virtual const char* typeName() const = 0;
};

template <typename T>
struct Payload : public PayloadBase
{
    Payload( T p ) { payload = p; }
    Payload( const Payload& other )
    {
       payload = other.payload;
    }
    Payload & operator=( const Payload & other )
    {
       payload = other.payload;
    }

    PayloadBase * clone() const
    {
        return new Payload<T>( const_cast<Payload<T>* >(this)->payload);
    }

    const char* typeName() const
    {
      return typeid(const_cast<Payload<T>*> (this)).name();
    }

    T payload;
};

template <typename T>
struct Payload<T*> : public PayloadBase
{
    Payload( T* )
    {
        Q_ASSERT_X( false, "Akonadi::Payload", "The Item class is not intended to be used with raw pointer types. Please use a smart pointer instead." );
    }
};


