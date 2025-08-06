package com.mslabs.pineda.vulkanandroid.ml

object ModelMapper {
    private val labelToModelMap = mapOf(
        "beagle" to "Beagle",
        "border_collie" to "Border Collie",
        "boxer" to "Boxer",
        "french_bulldog" to "French Bulldog",
        "golden_retriever" to "Golden Retriever",
        "siberian_husky" to "Husky",
        "labrador_retriever" to "Labrador",
        "pug" to "Pug",
        "rottweiler" to "Rottweiler",
        "german_shepherd" to "Shepherd",
        "doberman" to "Doberman",
        "pomeranian" to "Pomeranian Spitz",
        "toy_terrier" to "Toy Terrier",
        "pembroke" to "Corgi",
        "cardigan" to "Corgi",
        "akita_inu" to "Husky",
        "bulldog" to "French Bulldog",
        "dalmatian" to "Labrador",
        "jack_russell_terrier" to "Toy Terrier",
        "pitbull" to "Boxer",
        "shiba_inu" to "Pomeranian Spitz"
    )

    fun getModelForLabel(label: String): String? {
        return labelToModelMap[label.lowercase()]
    }
}