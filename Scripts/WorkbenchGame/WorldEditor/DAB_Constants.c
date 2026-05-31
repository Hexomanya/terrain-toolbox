#ifdef WORKBENCH

class DAB_Constants
{
	// For float comparing
	static const float EPSILON = 1e-4;
	static const float EPSILONSQ = 1e-8;
	
	// String variable names
	static const string ANGLES = "angles";
	static const string COORDS = "coords";
	static const string PLACEMENT = "placement";
	static const string FLAGS = "Flags";
	static const string MATSORTBIAS = "MatSortBias";
	static const string ROADSORT = "RoadSort";
	static const string SCALE = "scale";
	
	// String variable values
	static const int NONE = 0;
	static const int SLOPEANDCONTACT = 1;
	static const int SLOPEX = 2;
	static const int SLOPEY = 3;
}

#endif // WORKBENCH
