enum BandType {
	BT_None = 0,
	Deadband0 = 1,  	// Block unless value changes is greater than or equal to
	Deadband1 = 2,		// Block unless value changes is greater than
	Narrowband0 = 3,	// Block if value changes is greater than or equal to
	Narrowband1 = 4		// Block if value changes is greater than
};

/*
 * Band
 */
class Band
{
  public:
	  Band() {};
	  virtual ~Band() {};

		void reset(BandType type, uint16_t gap, bool pct);

		bool status = false; // indicate this measure is to go
    bool check(double measure); // abstract function, check if need pass the measure.

  private:
    BandType _type = BT_None;
		uint16_t _gap = 0;
		bool _pct = 0;		// percentage or fixed value
		float _last = 0; 	// old value
};