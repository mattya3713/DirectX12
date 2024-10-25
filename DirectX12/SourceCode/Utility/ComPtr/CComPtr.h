#pragma once
#include <wrl/client.h>
#include <utility> // std::swap ���g�p���邽�߂ɃC���N���[�h

template <typename T>
class MyComPtr {
public:
    // �f�t�H���g�R���X�g���N�^.
    MyComPtr() : m_ptr(nullptr) {}

    // �|�C���^���󂯎��R���X�g���N�^.
    explicit MyComPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    // �f�X�g���N�^.
    ~MyComPtr() {
        Reset();
    }

    // �R�s�[�R���X�g���N�^�͍폜.
    MyComPtr(const MyComPtr&) = delete;

    // ���[�u�R���X�g���N�^.
    MyComPtr(MyComPtr&& other) noexcept : m_ptr(other.m_ptr) {
        // �ړ����𖳌��ɂ���.
        other.m_ptr = nullptr; 
    }

    // �R�s�[������Z�q�͍폜.
    MyComPtr& operator=(const MyComPtr&) = delete;

    // ���[�u������Z�q.
    MyComPtr& operator=(MyComPtr&& other) noexcept {
        if (this != &other) {
            // ���݂̃|�C���^�����.
            Reset(); 
            // �V�����|�C���^���ړ�.
            m_ptr = other.m_ptr;
            // �ړ����𖳌��ɂ���.
            other.m_ptr = nullptr; 
        }
        return *this;
    }

    // �����|�C���^�̎擾.
    T* Get() const { return m_ptr; }

    // �|�C���^�̃f���t�@�����X.
    T& operator*() const { return *m_ptr; }

    // �|�C���^�ւ̃A�N�Z�X.
    T* operator->() const { return m_ptr; }

    // �V�����|�C���^��ݒ肷��.
    void Reset(T* ptr = nullptr) {

        if (m_ptr) {
            // ���݂̃|�C���^�����.
            m_ptr->Release(); 
        }

        m_ptr = ptr;

        if (m_ptr) {
            // �V�����|�C���^���Q�ƃJ�E���g.
            m_ptr->AddRef();
        }
    }

    // �|�C���^����������.
    void Swap(MyComPtr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    // �����|�C���^�̃A�h���X���擾����.
    T** GetAddressOf() {

        // �ȑO�̃|�C���^�����.
        Reset();

        // �����|�C���^�̃A�h���X��Ԃ�.
        return &m_ptr;     
    }

    // ���݂̃|�C���^��������A���̃A�h���X��Ԃ�.
    T** ReleaseAndGetAddressOf() {

        // ���݂̃|�C���^�̃A�h���X��ێ�.
        T** address = &m_ptr;

        Reset(); // ���݂̃|�C���^�����.

        // ��������|�C���^�̃A�h���X��Ԃ�.
        return address;
    }

private:
    T* m_ptr; // COM �I�u�W�F�N�g�̃|�C���^.
};