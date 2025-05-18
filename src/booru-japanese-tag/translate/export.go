package translate

import (
	"encoding/csv"
	"github.com/boorutan/booru-japanese-tag/db"
	"os"
)

func ExportTagCompleteTranslateFile() error {
	rows, _ := db.DB.Query("SELECT id, name, post_count, alias, translated_name FROM tag WHERE translated = true ORDER BY post_count DESC")
	var recodes [][]string
	for rows.Next() {
		tag := Tag{}
		_ = rows.Scan(&tag.Id, &tag.Name, &tag.PostCount, &tag.Alias, &tag.TranslatedName)
		tagStr := []string{tag.Name, tag.TranslatedName}
		recodes = append(recodes, tagStr)
	}
	_ = os.Remove("danbooru-jp.csv")
	file, err := os.Create("danbooru-jp.csv")
	if err != nil {
		return err
	}
	defer func(file *os.File) {
		_ = file.Close()
	}(file)
	writer := csv.NewWriter(file)
	for _, recode := range recodes {
		_ = writer.Write(recode)
	}
	writer.Flush()
	return err
}

func ExportTagWithMachineTranslate() error {
	_ = os.Remove("danbooru-machine-jp.csv")
	file, err := os.Create("danbooru-machine-jp.csv")
	if err != nil {
		return err
	}
	defer func(file *os.File) {
		_ = file.Close()
	}(file)
	writer := csv.NewWriter(file)
	rows, _ := db.DB.Query("SELECT name, translated_name FROM tag WHERE translated = true ORDER BY post_count DESC")
	for rows.Next() {
		tag := Tag{}
		_ = rows.Scan(&tag.Name, &tag.TranslatedName)
		_ = writer.Write([]string{tag.Name, tag.TranslatedName})
	}
	rows, _ = db.DB.Query("SELECT name, machine_translated_name FROM tag WHERE translated = false ORDER BY post_count DESC")
	for rows.Next() {
		tag := Tag{}
		_ = rows.Scan(&tag.Name, &tag.MachineTranslatedName)
		_ = writer.Write([]string{tag.Name, tag.MachineTranslatedName})
	}
	writer.Flush()
	return err
}
