package main

import (
	"fmt"
	"github.com/boorutan/booru-japanese-tag/db"
	"github.com/boorutan/booru-japanese-tag/translate"
	"github.com/charmbracelet/bubbles/list"
	"github.com/charmbracelet/bubbles/textinput"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"github.com/muesli/termenv"
	"strconv"
)

type item struct {
	title, desc string
}

func (i item) Title() string       { return i.title }
func (i item) Description() string { return i.desc }
func (i item) FilterValue() string { return i.title }

type model struct {
	list          list.Model
	command       int
	translateTag  translate.Tag
	translatedTag textinput.Model
	isSearchTag   bool
	searchTag     textinput.Model
}

func (m model) Init() tea.Cmd {
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		if msg.String() == "ctrl+c" {
			return m, tea.Quit
		}
		if msg.String() == "enter" {
			if m.command == 0 {
				m.command = m.list.Index() + 1
				if m.command == 1 {
					tag := translate.GetTag()
					m.translateTag = tag
					m.translatedTag.Focus()
					break
				}
				if m.command == 2 {
					m.isSearchTag = true
					m.searchTag.Focus()
					break
				}
			}
			if m.command == 1 {
				value := m.translatedTag.Value()
				if len(value) != 0 {
					_ = translate.UpdateTag(m.translateTag.Name, value)
				}
				tag := translate.GetTag()
				m.translateTag = tag
				m.translatedTag.Reset()
			}
			if m.command == 2 {
				if m.isSearchTag {
					tagName := m.searchTag.Value()
					if len(tagName) == 0 {
						break
					}
					tag, err := translate.Tag{
						Name: tagName,
					}.GetTag()
					if err != nil {
						break
					}
					m.translateTag = tag
					m.isSearchTag = false
					m.translatedTag.Focus()
				} else {
					value := m.translatedTag.Value()
					if len(value) != 0 {
						_ = translate.UpdateTag(m.translateTag.Name, value)
					}
					m.isSearchTag = true
					m.searchTag.Reset()
					m.translatedTag.Reset()
					m.searchTag.Focus()
				}
			}
			if m.command == 3 {
				err := translate.ExportTagCompleteTranslateFile()
				if err != nil {
					println(err.Error())
					return m, tea.Quit
				}
				return m, tea.Quit
			}
			if m.command == 4 {
				err := translate.ExportTagWithMachineTranslate()
				if err != nil {
					println(err.Error())
					return m, tea.Quit
				}
				return m, tea.Quit
			}
			if m.command == 5 {
				err := translate.ImportMachineTranslatedDanbooruTag()
				if err != nil {
					return m, tea.Quit
				}
				return m, tea.Quit
			}
			if m.command == 6 {
				err := translate.ImportDanbooruTag()
				if err != nil {
					println(err.Error())
					return m, tea.Quit
				}
				return m, tea.Quit
			}
		}
	case tea.WindowSizeMsg:
		var docStyle = lipgloss.NewStyle().Margin(1, 2)
		h, v := docStyle.GetFrameSize()
		m.list.SetSize(msg.Width-h, msg.Height-v)
	}

	var cmd tea.Cmd
	if m.command == 0 {
		m.list, cmd = m.list.Update(msg)
	}
	if m.command == 1 {
		m.translatedTag, cmd = m.translatedTag.Update(msg)
	}
	if m.command == 2 {
		if m.isSearchTag {
			m.searchTag, cmd = m.searchTag.Update(msg)
		} else {
			m.translatedTag, cmd = m.translatedTag.Update(msg)
		}
	}
	return m, cmd
}

func (m model) View() string {
	var docStyle = lipgloss.NewStyle().Margin(1, 2)
	var color = termenv.EnvColorProfile().Color
	var keyword = termenv.Style{}.Foreground(color("204")).Background(color("235")).Bold().Underline().Styled
	var bold = termenv.Style{}.Bold().Styled
	var underline = termenv.Style{}.Bold().Underline().Styled
	if m.command == 0 {
		return docStyle.Render(m.list.View())
	}
	if m.command == 1 {
		return fmt.Sprintf(
			"\n\n %s > %s\n\nCount: %s\nAlias: %s\n",
			keyword(m.translateTag.Name),
			bold(m.translatedTag.View()[2:]),
			underline(strconv.Itoa(m.translateTag.PostCount)),
			bold(m.translateTag.Alias),
		)
	}
	var translated = (func() string {
		if len(m.translateTag.TranslatedName) != 0 {
			return fmt.Sprintf("Translate: %s\n", keyword(m.translateTag.TranslatedName))
		}
		return ""
	})()
	return fmt.Sprintf(
		"\n\n %s > %s\n\nCount: %s\nAlias: %s\n%s",
		keyword(m.searchTag.View()[2:]),
		bold(m.translatedTag.View()[2:]),
		underline(strconv.Itoa(m.translateTag.PostCount)),
		bold(m.translateTag.Alias),
		translated,
	)
}

func main() {
	db.InitDB()

	items := []list.Item{
		item{title: "単語を翻訳する", desc: "Dannboruのタグを日本語翻訳します"},
		item{title: "特定の単語を翻訳する", desc: "特定のDannboruのタグを日本語翻訳します"},
		item{title: "単語をエクスポートする", desc: "今までした翻訳をエクスポートします"},
		item{title: "機械翻訳と一緒に単語をエクスポートする", desc: "今までした翻訳を機械学習と一緒にエクスポートします"},
		item{title: "機械翻訳をインポートする", desc: "機械翻訳をインポートします、環境によっては1分以上かかります"},
		item{title: "単語をインポートする", desc: "翻訳する単語をインポートします、環境によっては1分以上かかります"},
	}

	m := model{
		list:          list.New(items, list.NewDefaultDelegate(), 0, 0),
		translatedTag: textinput.New(),
		isSearchTag:   false,
		searchTag:     textinput.New(),
	}
	m.list.Title = "Command"
	m.translatedTag.Placeholder = "女の子"

	p := tea.NewProgram(m, tea.WithAltScreen())

	_, err := p.Run()
	if err != nil {
		return
	}
}
