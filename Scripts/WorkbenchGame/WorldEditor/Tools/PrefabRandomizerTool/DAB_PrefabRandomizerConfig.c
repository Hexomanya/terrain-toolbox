#ifdef WORKBENCH // this config is only related to a Workbench tool (DAB_PrefabRadomizerTool)
[BaseContainerProps(configRoot: true)]
class DAB_PrefabRandomizerConfig
{
	[Attribute(defvalue: "Unnamed Config", desc: "Name of the Config")]
	string m_configName;
	
	[Attribute(defvalue: "", desc: "Config for one object replacement")]
	ref array<ref DAB_PrefabRandomizerConfigEntry> m_replacementEntries;
	
	[Attribute(defvalue: "false", desc: "Do not randomize. Step trough the list and loop when at end")]
	bool m_replaceInOrder;

	//------------------------------------------------------------------------------------------------
	// constructor
	void DAB_PrefabRandomizerConfig()
	{
		if (!m_replacementEntries) m_replacementEntries = {};
	}
}

[BaseContainerProps()]
class DAB_PrefabRandomizerConfigEntry
{
	[Attribute(desc: "Prefabs that will be selected and replaced", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ref array<ResourceName> m_replacedPrefabs;
	
	[Attribute(desc: "Weight in replacement randomisation (more weight = more chance to be selected)", defvalue: "1", params: "0 1 0.05")]
	float m_randomizeProbability;
	
	[Attribute(desc: "Prefabs that will preplace selected ones", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ref array<ResourceName> m_replacerPrefabs;
	[Attribute(defvalue: "1", desc: "Weights for the replacement Prefabs")]
	ref array<float> m_replacerWeights;
}

#endif // WORKBENCH