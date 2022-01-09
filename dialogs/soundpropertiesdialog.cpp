#include "soundpropertiesdialog.h"
#include "ui_soundpropertiesdialog.h"

#include <QFileDialog>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>

#include "projectcontainer.h"

SoundPropertiesDialog::SoundPropertiesDialog(Sound &sound, QWidget *parent) :
    QDialog{parent},
    m_ui{std::make_unique<Ui::SoundPropertiesDialog>()},
    m_sound{sound}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Sound Properties: %0").arg(m_sound.name));

    m_ui->lineEditName->setText(m_sound.name);
    if (!m_sound.path.isEmpty())
    {
        m_ui->labelFilename->setText(tr("Filename: %0").arg(QFileInfo{m_sound.path}.fileName()));
        m_soundEffect.setSource(QUrl::fromLocalFile(m_sound.path));
    }
    m_ui->radioButtonNormal->setChecked(m_sound.type == Sound::Type::Sound);
    m_ui->radioButtonMusic->setChecked(m_sound.type == Sound::Type::Music);
    m_ui->checkBoxChorus->setChecked(m_sound.effects.chorus);
    m_ui->checkBoxFlanger->setChecked(m_sound.effects.flanger);
    m_ui->checkBoxGargle->setChecked(m_sound.effects.gargle);
    m_ui->checkBoxEcho->setChecked(m_sound.effects.echo);
    m_ui->checkBoxReverb->setChecked(m_sound.effects.reverb);
    m_ui->horizontalSliderVolume->setValue(m_sound.volume);
    m_ui->horizontalSliderPan->setValue(m_sound.pan);
    m_ui->checkBoxPreload->setChecked(m_sound.preload);

    connect(m_ui->pushButtonLoad, &QAbstractButton::pressed,
            this, &SoundPropertiesDialog::loadSound);
    connect(m_ui->pushButtonPlay, &QAbstractButton::pressed,
            this, &SoundPropertiesDialog::playSound);
    connect(m_ui->pushButtonStop, &QAbstractButton::pressed,
            this, &SoundPropertiesDialog::stopSound);
    connect(m_ui->pushButtonSave, &QAbstractButton::pressed,
            this, &SoundPropertiesDialog::saveSound);
    connect(m_ui->pushButtonEdit, &QAbstractButton::pressed,
            this, &SoundPropertiesDialog::editSound);

    connect(m_ui->lineEditName, &QLineEdit::textChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->radioButtonNormal, &QRadioButton::toggled,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->radioButtonMusic, &QRadioButton::toggled,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->radioButton3D, &QRadioButton::toggled,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->radioButtonMultimedia, &QRadioButton::toggled,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxChorus, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxFlanger, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxGargle, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxEcho, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxReverb, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->horizontalSliderVolume, &QSlider::valueChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->horizontalSliderPan, &QSlider::valueChanged,
            this, &SoundPropertiesDialog::changed);
    connect(m_ui->checkBoxPreload, &QCheckBox::stateChanged,
            this, &SoundPropertiesDialog::changed);
}

SoundPropertiesDialog::~SoundPropertiesDialog() = default;

void SoundPropertiesDialog::accept()
{
    if (m_sound.name != m_ui->lineEditName->text())
    {
        QMessageBox::critical(this, tr("Not implemented"), tr("Changing the name is not yet implemented!"));
        return;
    }

    if (m_newPath)
        m_sound.path = *m_newPath;

    if (m_ui->radioButtonNormal->isChecked())
        m_sound.type = Sound::Type::Sound;
    else if (m_ui->radioButtonMusic->isChecked())
        m_sound.type = Sound::Type::Music;
    else
    {
        QMessageBox::critical(this, tr("Not implemented"), tr("This kind of sound is not yet supported!"));
        return;
    }

    m_sound.effects.chorus = m_ui->checkBoxChorus->isChecked();
    m_sound.effects.flanger = m_ui->checkBoxFlanger->isChecked();
    m_sound.effects.gargle = m_ui->checkBoxGargle->isChecked();
    m_sound.effects.echo = m_ui->checkBoxEcho->isChecked();
    m_sound.effects.reverb = m_ui->checkBoxReverb->isChecked();
    m_sound.volume = m_ui->horizontalSliderVolume->value();
    m_sound.pan = m_ui->horizontalSliderPan->value();

    QDialog::accept();
}

void SoundPropertiesDialog::reject()
{
    if (!m_unsavedChanges)
    {
        QDialog::reject();
        return;
    }

    const auto result = QMessageBox::warning(
        this,
        tr("The Sound has been modified."),
        tr("Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );
    switch (result)
    {
    case QMessageBox::Save:
        accept();
        return;
    case QMessageBox::Discard:
        QDialog::reject();
        return;
    case QMessageBox::Cancel:
        return;
    default:
        qWarning() << "unexpected dialog result" << result;
    }
}

void SoundPropertiesDialog::loadSound()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Open a Sound File..."));
    if (path.isEmpty())
        return;

    if (QFile file{path}; !file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Could not load Sound!"), tr("Could not load Sound!") + "\n\n" + file.errorString());
        return;
    }

    m_newPath = path;
    changed();
    m_ui->labelFilename->setText(tr("Filename: %0").arg(QFileInfo{path}.fileName()));
    m_soundEffect.setSource(QUrl::fromLocalFile(path));
}

void SoundPropertiesDialog::saveSound()
{
    const auto &path = m_newPath ? *m_newPath : m_sound.path;
    if (path.isEmpty())
    {
        QMessageBox::warning(this, tr("Could not save Sound!"), tr("Could not save Sound!") + "\n\n" + tr("No sound has been selected yet."));
        return;
    }

    const auto savePath = QFileDialog::getSaveFileName(this, tr("Save a Sound File..."), m_sound.name + ".wav", tr("WAV Files (*.wav)"));
    if (savePath.isEmpty())
        return;

    if (!QFile::copy(path, savePath))
    {
        QMessageBox::warning(this, tr("Could not save Sound!"), tr("Could not save Sound!"));
        return;
    }
}

void SoundPropertiesDialog::playSound()
{
    m_soundEffect.play();
}

void SoundPropertiesDialog::stopSound()
{
    m_soundEffect.stop();
}

void SoundPropertiesDialog::editSound()
{
    QMessageBox::critical(this, tr("Setup not complete"), tr("No valid external editor has been indicated for this type of sound. You can specify this editor in the Preferences."));
}

void SoundPropertiesDialog::changed()
{
    if (!m_unsavedChanges)
    {
        setWindowTitle(tr("Sound Properties: %0*").arg(m_sound.name));
        m_unsavedChanges = true;
    }
}