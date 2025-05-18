package translate

import (
	"encoding/csv"
	"github.com/boorutan/booru-japanese-tag/db"
	"math/rand"
	"os"
)

type Tag struct {
	Name                  string
	PostCount             int
	Alias                 string
	Id                    int
	TranslatedName        string
	Translated            bool
	MachineTranslatedName string
}

func GetTag() Tag {
	rows, _ := db.DB.Query("SELECT id, name, post_count, alias FROM tag WHERE translated = false AND category = 0 OR category = 4 ORDER BY post_count DESC LIMIT 5")
	var tags []Tag
	for rows.Next() {
		tag := Tag{}
		_ = rows.Scan(&tag.Id, &tag.Name, &tag.PostCount, &tag.Alias)
		tags = append(tags, tag)
	}
	return tags[rand.Intn(len(tags))]
}

func (t Tag) GetTag() (Tag, error) {
	row := db.DB.QueryRow("SELECT id, name, post_count, alias, translated FROM tag WHERE name = ?", t.Name)
	var tag Tag
	err := row.Scan(&tag.Id, &tag.Name, &tag.PostCount, &tag.Alias, &tag.Translated)
	if tag.Translated {
		row := db.DB.QueryRow("SELECT translated_name FROM tag WHERE name = ?", t.Name)
		_ = row.Scan(&tag.TranslatedName)
	}
	if err != nil {
		return tag, err
	}
	return tag, nil
}

func UpdateTag(en, ja string) error {
	_, err := db.DB.Exec("UPDATE tag SET translated = true, translated_name = ? WHERE name = ?", ja, en)
	return err
}

func ImportMachineTranslatedDanbooruTag() error {
	file, err := os.Open("danbooru-only-machine-jp.csv")
	if err != nil {
		return err
	}
	defer func(file *os.File) {
		_ = file.Close()
	}(file)
	reader := csv.NewReader(file)
	rows, err := reader.ReadAll()
	if err != nil {
		return err
	}
	for _, v := range rows {
		_, err = db.DB.Exec("UPDATE tag SET machine_translated_name = ? WHERE name = ?", v[1], v[0])
		if err != nil {
			continue
		}
	}
	return nil
}

func ImportDanbooruTag() error {
	_, _ = db.Execute("DELETE FROM tag WHERE translated = false")
	file, err := os.Open("danbooru.csv")
	if err != nil {
		return err
	}
	defer func(file *os.File) {
		_ = file.Close()
	}(file)
	reader := csv.NewReader(file)
	rows, err := reader.ReadAll()
	if err != nil {
		return err
	}
	for _, v := range rows {
		_, err = db.DB.Exec(
			"INSERT INTO tag (name, category, post_count, alias, translated) VALUES (?, ?, ?, ?, false)",
			v[0],
			v[1],
			v[2],
			v[3],
		)
		if err != nil {
			continue
		}
	}
	return nil
}
