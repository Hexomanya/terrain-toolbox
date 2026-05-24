class DAB_ArrayHelper<Class T>
{
	//------------------------------------------------------------------------------------------------
	static array<T> Combine(notnull array<T> arrayA, notnull array<T> arrayB)
	{
		array<T> combined = {};
		combined.InsertAll(arrayA);
		combined.InsertAll(arrayB);
		
		return combined;
	}
}